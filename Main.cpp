#include "Bang/Application.h"

#include "Bang/File.h"
#include "Bang/Mesh.h"
#include "Bang/Font.h"
#include "Bang/Scene.h"
#include "Bang/Shader.h"
#include "Bang/Resources.h"
#include "Bang/Transform.h"
#include "Bang/SystemUtils.h"
#include "Bang/MeshFactory.h"
#include "Bang/ImageEffects.h"
#include "Bang/SceneManager.h"
#include "Bang/ShaderProgram.h"
#include "Bang/Framebuffer.h"
#include "Bang/RenderTexture.h"
#include "Bang/FontSheetCreator.h"

using namespace Bang;

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
    constexpr int loadCharSize = 256;
    constexpr int radius = loadCharSize / 2;
    constexpr int outputCharSize = 128;
    constexpr float signedOffset = 0.25f;

    Font *font = new Font();
    font->SetLoadSize(loadCharSize);
    font->Import(ttfPath);

    String chars = "";
    chars += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    chars += "abcdefghijklmnopqrstuvwxyz";
    chars += "0123456789.,-;:_^-+*[]{}()?!%&/\\=><#@ ";

    const GL::Attachment color0 = GL::Attachment::Color0;
    const GL::Attachment color1 = GL::Attachment::Color1;
    const Array<GL::Attachment> colorArr0 = {color0};
    const Array<GL::Attachment> colorArr1 = {color1};;

    Array<Imageb> charDistFieldImages;
    Array<Vector2i> charSpreadOffsets;
    for (int i = 0; i < chars.Size(); ++i)
    {
        const char c = chars[i];
        Debug_Log("Processing char '" << c <<
                  "' (" << (i+1) << "/" << chars.Size() << ")");

        Texture2D charBitmap;
        if (!FontSheetCreator::LoadAtlasTexture(font->GetTTFFont(),
                                                  &charBitmap, String(c),
                                                  nullptr, 0))
        {
            Debug_Error("Could not load atlas texture '" << ttfPath << "' for char"
                        "'" << c << "'.");
            return 1;
        }

        Imageb charBitmapImg = charBitmap.ToImage(false);
        Vector2i charSize = charBitmapImg.GetSize();
        charSize = Vector2i::Max(charSize, Vector2i::One);
        // charBitmapImg.Export( Path("Char_" + String(int(c)) + ".png") );

        Imageb charImgMargined = charBitmapImg;
        charImgMargined.AddMargins(Vector2i(radius+1), Color::Zero,
                                   ImageAspectRatioMode::KeepExceeding);
        // charImgMargined.Export( Path("Char_" + String(int(c)) + "M.png") );

        Imageb charOutlineImg;
        ImageEffects::Outline(charImgMargined, &charOutlineImg);

        Texture2D charOutline;
        charOutline.Import(charOutlineImg);

        Framebuffer framebuffer(charImgMargined.GetWidth(),
                                  charImgMargined.GetHeight());
        framebuffer.Bind();
        framebuffer.CreateColorAttachment(color0, GL::ColorFormat::RGBA_Float16);
        framebuffer.CreateColorAttachment(color1, GL::ColorFormat::RGBA_Float16);
        framebuffer.SetAllDrawBuffers();
        framebuffer.ClearColor(Color::Zero);

        // First pass
        ShaderProgram spFirstPass;
        spFirstPass.Load( Path("pass.vert"), Path("firstPass.frag") );
        framebuffer.SetDrawBuffers(colorArr0);
        spFirstPass.Bind();
        spFirstPass.Set("ImgSize", Vector2(charOutlineImg.GetSize()));
        spFirstPass.Set("OutlineTex", &charOutline);
        DoPass();
        spFirstPass.UnBind();

        // Iterative passes, to increase distance boundary
        ShaderProgram spPass;
        spPass.Load( Path("pass.vert"), Path("pass.frag") );
        spPass.Bind();
        spPass.Set("ImgSize", Vector2(charImgMargined.GetSize()));
        bool pingPong = false;
        for (int i = 0; i < radius; ++i)
        {
            framebuffer.SetDrawBuffers(pingPong ? colorArr0 : colorArr1);
            spPass.Set("DistField", pingPong ? framebuffer.GetAttachmentTexture(color1) :
                                               framebuffer.GetAttachmentTexture(color0));
            DoPass();
            pingPong = !pingPong;
        }
        spPass.UnBind();

        RenderTexture *charDistFieldTex = framebuffer.GetAttachmentTexture(
                    framebuffer.GetCurrentDrawAttachments().Front());
        Imagef charDFImg = charDistFieldTex->ToImage<float>();

        Vector2i minPixel = charDFImg.GetSize() - Vector2i::One;
        Vector2i maxPixel = Vector2i::Zero;
        for (int y = 0; y < charDFImg.GetHeight(); ++y)
        {
            for (int x = 0; x < charDFImg.GetWidth(); ++x)
            {
                Color color = charDFImg.GetPixel(x,y);
                bool isBackground = (color.a == 0.0f);
                if (!isBackground)
                {
                    Vector2 offsetXY(color.r, color.g);
                    float dist = offsetXY.Length() / radius;
                    bool isInterior = (charImgMargined.GetPixel(x,y).a > 0);
                    if (isInterior) { dist = -dist; }
                    dist += signedOffset;
                    isBackground = (dist >= 1.0f-signedOffset);
                    if (!isBackground)
                    {
                        dist = Math::Clamp(dist, 0.0f, 1.0f);
                        charDFImg.SetPixel(x,y, Color(dist, dist, dist,1));

                        Vector2i xy(x,y);
                        minPixel = Vector2i::Min(minPixel, xy);
                        maxPixel = Vector2i::Max(maxPixel, xy);
                        // if (isInterior) { charDFImg.SetPixel(x,y,Color::Green); }
                    }
                }

                if (isBackground)
                {
                    charDFImg.SetPixel(x,y, Color::White);
                }
            }
        }

        Vector2i dfFittedSize = (maxPixel-minPixel);
        if (dfFittedSize.x > 0 && dfFittedSize.y > 0)
        {
            Imageb charDFImgFitted(dfFittedSize.x, dfFittedSize.y);
            charDFImgFitted.Copy(charDFImg.To<Byte>(),
                                 Recti(minPixel, maxPixel),
                                 Recti(Vector2i::Zero, dfFittedSize));

            // charDFImgFitted.Export( Path("DistField_" + String(int(c)) + "_FITTED_0.png") );
            Imageb charDFImgFittedSmall = charDFImgFitted;
            charDFImgFittedSmall.Resize(Vector2i(outputCharSize),
                                        ImageResizeMode::Linear,
                                        ImageAspectRatioMode::Keep);
            // charDFImgFittedSmall.Export( Path("DistField_" + String(int(c)) + "_FITTED_1.png") );
            charDistFieldImages.PushBack( charDFImgFittedSmall );

            Vector2 charScalingDown(Vector2(charDFImgFittedSmall.GetSize()) /
                                    Vector2(charDFImgFitted.GetSize()));
            Vector2 spreadOffset((dfFittedSize - charSize) / 2);
            spreadOffset *= charScalingDown;
            charSpreadOffsets.PushBack( Vector2i(spreadOffset) );
        }
        else
        {
            Imageb empty;
            empty.Create(1, 1, Color::Zero);
            charDistFieldImages.PushBack(empty);
            charSpreadOffsets.PushBack(Vector2i::Zero);
        }

        framebuffer.UnBind();
    }

    Array<Recti> imageOutputRects;
    Imageb fontDistFieldImg =
            FontSheetCreator::PackImages(charDistFieldImages, 5,
                                         &imageOutputRects,
                                         Color::White);

    String fileName = ttfPath.GetName() + "_DistField";
    fontDistFieldImg.Export( Path(fileName).AppendExtension("png") );

    XMLNode xmlInfo;
    for (int i = 0; i < chars.Size(); ++i)
    {
        const unsigned char c = chars[i];
        const Vector2i& spreadOffset = charSpreadOffsets[i];
        xmlInfo.Set("SpreadOffset_" + String(int(c)), spreadOffset);

        const Recti &charDFRect = imageOutputRects[i];
        Recti actualCharRect = Recti(charDFRect.GetMin() + spreadOffset,
                                     charDFRect.GetMax() - spreadOffset);
        xmlInfo.Set("CharRect_" + String(int(c)), actualCharRect);

        // fontDistFieldImg.GetSubImage(charDFRect).Export(Path("NotActual_" + String(i) + ".png"));
        // fontDistFieldImg.GetSubImage(actualCharRect).Export(Path("Actual_" + String(i) + ".png"));
    }
    xmlInfo.Set("LoadSize", font->GetLoadSize());
    xmlInfo.Set("SignedOffset", signedOffset);
    File::Write(Path(fileName).AppendExtension("info"), xmlInfo.ToString());

    return 0;
}
