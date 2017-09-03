#version 130

uniform vec2 ImgSize;
uniform sampler2D OutlineTex;

void main()
{
    vec2 uvStep = 1.0f / ImgSize;
    vec2 uv = uvStep * gl_FragCoord.xy;

    vec4 outlineColor = texture2D(OutlineTex, uv);
    bool isOutline = outlineColor.a > 0;
    // gl_FragColor = outlineColor;
    gl_FragColor = vec4(0, 0, 0, isOutline ? 1 : 0);
}

