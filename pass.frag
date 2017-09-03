#version 130

uniform vec2 ImgSize;
uniform sampler2D DistField;
uniform sampler2D OutlineTex;

void main()
{
    vec2 uvStep = 1.0f / ImgSize;
    vec2 uv = uvStep * gl_FragCoord.xy;

    vec2 closestBorderOffset;
    float minDist = 999999.9f;
    bool replaceColor = false;
    vec4 thisColor = texture2D(DistField, uv);
    if (thisColor.a > 0)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                if (x == 0 && y == 0) { continue; }

                vec2 uvXY = uvStep * vec2(x,y);
                vec4 neighbour = texture2D(DistField, uv + uvXY);
                if (neighbour.a > 0)
                {
                    vec2 neighbourClosestBorderOffset = neighbour.xy;
                    vec2 borderOffset = neighbourClosestBorderOffset + uvXY;
                    float dist = length(borderOffset);
                    if (dist < minDist)
                    {
                        minDist = dist;
                        replaceColor = true;
                        closestBorderOffset = borderOffset;
                    }
                }
            }
        }
    }

    if (replaceColor)
    {
        if (minDist == 0) { gl_FragColor = vec4(0,1,0,1); }
        else { gl_FragColor = vec4(closestBorderOffset, minDist, 1); }
    }
    else
    {
        gl_FragColor = thisColor;
    }
}

