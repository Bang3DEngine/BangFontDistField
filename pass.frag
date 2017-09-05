#version 130

uniform vec2 ImgSize;
uniform sampler2D DistField;

void main()
{
    vec2 uvStep = 1.0f / ImgSize;
    vec2 uv = uvStep * gl_FragCoord.xy;

    vec2 closestBorderOffset;
    float minDist = 999999.9f;
    bool replaceColor = false;
    vec4 thisColor = texture2D(DistField, uv);
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            if (x == 0 && y == 0) { continue; }

            vec2 xy = vec2(x,y);
            vec2 uvXY = uvStep * xy;
            vec4 neighbour = texture2D(DistField, uv + uvXY);
            if (neighbour.a > 0)
            {
                vec2 neighbourClosestBorderOffset = neighbour.xy;
                vec2 borderOffset = neighbourClosestBorderOffset + xy;
                float dist = pow(borderOffset.x, 2) + pow(borderOffset.y, 2);
                if (dist < minDist)
                {
                    minDist = dist;
                    replaceColor = true;
                    closestBorderOffset = borderOffset;
                }
            }
        }
    }

    if (replaceColor)
    {
        gl_FragColor = vec4(closestBorderOffset, 0, 1);
    }
    else
    {
        gl_FragColor = thisColor;
    }
}

