#include "Bang/Application.h"

#include "Bang/File.h"
#include "Bang/Mesh.h"
#include "Bang/Scene.h"
#include "Bang/G_Font.h"
#include "Bang/G_Shader.h"
#include "Bang/Resources.h"
#include "Bang/Transform.h"
#include "Bang/SystemUtils.h"
#include "Bang/MeshFactory.h"
#include "Bang/ImageEffects.h"
#include "Bang/SceneManager.h"
#include "Bang/ShaderProgram.h"
#include "Bang/G_Framebuffer.h"
#include "Bang/G_RenderTexture.h"
#include "Bang/G_FontSheetCreator.h"

void DoPass()
{
    Mesh *planeMesh = MeshFactory::GetUIPlane();
    GL::Render(planeMesh->GetVAO(),
               GL::Primitives::Triangles,
               planeMesh->GetPositions().Size());
}

int main(int argc, char **argv)
{
    Application app(argc, argv);
    app.CreateWindow();

    Path ttfPath = EPATH("Fonts/Ubuntu.ttf");
    constexpr int loadCharSize = 512;
    constexpr int radius = loadCharSize / 4;
    constexpr int outputCharSize = 128;
    constexpr float signedOffset = 0.25f;

    G_Font *font = new G_Font();
    font->SetLoadSize(loadCharSize);
    font->Import(ttfPath);

    String chars = "";
    chars += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    chars += "abcdefghijklmnopqrstuvwxyz";
    chars += "0123456789.,-;:_^-+*[]{}()?¿¡!%&/\\=><#@ ";

    const GL::Attachment color0 = GL::Attachment::Color0;
    const GL::Attachment color1 = GL::Attachment::Color1;
    const Array<GL::Attachment> colorArr0 = {color0};
    const Array<GL::Attachment> colorArr1 = {color1};

    Array<G_Image> charDistFieldImages;
    for (const char c : chars)
    {
        Debug_Log("Processing char '" << c << "'");

        G_Texture2D *charBitmap = new Texture2D();
        if (!G_FontSheetCreator::LoadAtlasTexture(font->GetTTFFont(),
                                                  charBitmap, String(c),
                                                  nullptr, radius + 1))
        {
            Debug_Error("Could not load atlas texture '" << ttfPath << "' for char"
                        "'" << c << "'.");
            return 1;
        }
        Vector2i charSize(charBitmap->GetWidth(), charBitmap->GetHeight());

        G_Framebuffer framebuffer(charSize.x, charSize.y);
        framebuffer.Bind();
        framebuffer.CreateColorAttachment(color0, GL::ColorFormat::RGBA_Float16);
        framebuffer.CreateColorAttachment(color1, GL::ColorFormat::RGBA_Float16);
        framebuffer.SetAllDrawBuffers();

        G_ImageG<Byte> charOutlineImg;
        G_ImageG<Byte> charBitmapImg = charBitmap->ToImage(false);
        ImageEffects::Outline(charBitmapImg, &charOutlineImg);

        Texture2D charOutline;
        charOutline.Import(charOutlineImg);
        // charOutlineImg.Export( Path("Outline.png") );

        framebuffer.ClearColor(Color::Zero);

        // First pass
        G_ShaderProgram *spFirstPass = new G_ShaderProgram();
        framebuffer.SetDrawBuffers(colorArr0);
        spFirstPass->Load( Path("pass.vert"), Path("firstPass.frag") );
        spFirstPass->Bind();
        spFirstPass->Set("ImgSize", Vector2(charBitmapImg.GetSize()));
        spFirstPass->Set("OutlineTex", &charOutline);
        DoPass();
        spFirstPass->UnBind();

        // Iterative passes, to increase distance boundary
        G_ShaderProgram spPass;
        spPass.Load( Path("pass.vert"), Path("pass.frag") );

        bool pingPong = false;
        spPass.Bind();
        spPass.Set("ImgSize", Vector2(charBitmapImg.GetSize()));
        spPass.Set("OutlineTex", &charOutline);
        for (int i = 0; i < radius; ++i)
        {
            framebuffer.SetDrawBuffers(pingPong ? colorArr0 : colorArr1);
            spPass.Set("DistField", pingPong ? framebuffer.GetAttachmentTexture(color1) :
                                               framebuffer.GetAttachmentTexture(color0));
            DoPass();
            pingPong = !pingPong;
        }
        spPass.UnBind();

        G_RenderTexture *charDistFieldTex = framebuffer.GetAttachmentTexture(
                    framebuffer.GetCurrentDrawAttachments().Front());
        G_Imagef charDistFieldImg = charDistFieldTex->ToImage<float>();

        for (int y = 0; y < charDistFieldImg.GetHeight(); ++y)
        {
            for (int x = 0; x < charDistFieldImg.GetWidth(); ++x)
            {
                Color color = charDistFieldImg.GetPixel(x,y);
                if (color.a == 1.0f)
                {
                    Vector2 offsetXY(color.r, color.g);
                    float dist = offsetXY.Length() / radius;
                    bool isInterior = (charBitmapImg.GetPixel(x,y).a > 0);
                    if (isInterior) { dist = -dist; }
                    dist += signedOffset;
                    dist = Math::Clamp(dist, 0.0f, 1.0f);
                    charDistFieldImg.SetPixel(x,y, Color(dist, dist, dist,1));
                }
                else
                {
                    charDistFieldImg.SetPixel(x,y, Color::White);
                }
            }
        }
        charDistFieldImg.Resize(Vector2i(outputCharSize), ImageResizeMode::Linear);
        charDistFieldImg.Export( Path("DistField_" + String(int(c)) + "s.png") );
        charDistFieldImages.PushBack( charDistFieldImg.To<Byte>() );

        framebuffer.UnBind();
    }

    Array<Recti> imageOutputRects;
    G_Image fontDistFieldImg =
            G_FontSheetCreator::PackImages(charDistFieldImages, 0,
                                           &imageOutputRects);

    String fileName = ttfPath.GetName() + "_DistField";
    fontDistFieldImg.Export( Path(fileName).AppendExtension("png") );

    XMLNode xmlInfo;
    for (int i = 0; i < chars.Size(); ++i)
    {
        const char c = chars[i];
        xmlInfo.Set("CharRect_" + String(int(c)), imageOutputRects[i]);
    }
    xmlInfo.Set("LoadSize", font->GetLoadSize());
    xmlInfo.Set("SignedOffset", signedOffset);
    File::Write(Path(fileName).AppendExtension("info"), xmlInfo.ToString());

    // Put every character in the final atlas
    SystemUtils::System("xdg-open DistField_65.png");

    return 0;
}
