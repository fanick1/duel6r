/*
* Copyright (c) 2006, Ondrej Danek (www.ondrej-danek.net)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Ondrej Danek nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "project.h"

#define D6_ELEV_TEXTURE         68
#define D6_ELEV_MAX             30
#define D6_ELEV_SPEED           0.03f

struct d6ELEVATOR_s
{
    float   X;
    float   Y;
    float   *AddX;
    float   *AddY;
    int     Sections;
    int     NowSec;
    float   CurPos;
    int     AddPos;
    int     *SecMax;
};

extern myUINT           *d6WpnTexture;
static int              d6Elevators;
static d6ELEVATOR_s     d6Elev[D6_ELEV_MAX];

void ELEV_Init (void)
{
    int     i;

    glBindTexture (GL_TEXTURE_2D, d6WpnTexture[D6_ELEV_TEXTURE]);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    for (i = 0; i < D6_ELEV_MAX; i++)
    {
        d6Elev[i].AddX = NULL;
        d6Elev[i].AddY = NULL;
        d6Elev[i].SecMax = NULL;
    }

    d6Elevators = 0;
}

void ELEV_Free (void)
{
    int     i;

    for (i = 0; i < d6Elevators; i++)
    {
        MY_Free (d6Elev[i].AddX);
        MY_Free (d6Elev[i].AddY);
        MY_Free (d6Elev[i].SecMax);
    }

    d6Elevators = 0;
}

void ELEV_Add (int *newElev, bool mirror)
{
    d6ELEVATOR_s    *e;
    float           dX, dY, d;
    int             i;

    if (d6Elevators >= D6_ELEV_MAX)
        return;

    e = &d6Elev[d6Elevators++];

    if (mirror)
        for (i = 0; i <= newElev[0]; i++)
            newElev[2 * i + 1] = d6World.Level.SizeX - 1 - newElev[2 * i + 1];

    e->Sections = newElev[0];
    e->NowSec = 0;
    e->CurPos = 0;
    e->AddPos = 1;
    e->X = (float) newElev[1];
    e->Y = (float) (d6World.Level.SizeY - newElev[2]);
    e->AddX = D6_MALLOC (float, e->Sections);
    e->AddY = D6_MALLOC (float, e->Sections);
    e->SecMax = D6_MALLOC (int, e->Sections);

    for (i = 0; i < e->Sections; i++)
    {
        dX = (float) (newElev[3 + 2 * i] - newElev[1 + 2 * i]);
        dY = (float) (newElev[2 + 2 * i] - newElev[4 + 2 * i]);
        d = (float) sqrt (dX * dX + dY * dY);

        e->SecMax[i] = (int) (d / D6_ELEV_SPEED);
        e->AddX[i] = dX / (float) e->SecMax[i];
        e->AddY[i] = dY / (float) e->SecMax[i];
    }
}

void ELEV_MoveAll (void)
{
    d6ELEVATOR_s    *e;
    int             i;

    for (i = 0; i < d6Elevators; i++)
    {
        e = &d6Elev[i];

        e->X += e->AddPos * e->AddX[e->NowSec] * g_app.frame_interval;
        e->Y += e->AddPos * e->AddY[e->NowSec] * g_app.frame_interval;
        e->CurPos += e->AddPos * g_app.frame_interval;

        if (e->CurPos >= e->SecMax[e->NowSec])
        {
            if (++e->NowSec >= e->Sections)
            {
                e->NowSec = e->Sections - 1;
                e->CurPos = (float)e->SecMax[e->NowSec];
                e->AddPos = -1;
            }
            else
                e->CurPos = 0;
        }
        else
            if (e->CurPos <= 0)
            {
                if (--e->NowSec < 0)
                {
                    e->NowSec = 0;
                    e->CurPos = 0;
                    e->AddPos = 1;
                }
                else
                    e->CurPos = (float)e->SecMax[e->NowSec];
            }
    }
}

void ELEV_DrawAll (void)
{
    int     i;
    float   X, Y;

    glBindTexture (GL_TEXTURE_2D, d6WpnTexture[D6_ELEV_TEXTURE]);
    glBegin (GL_QUADS);

    for (i = 0; i < d6Elevators; i++)
    {
        X = d6Elev[i].X;
        Y = d6Elev[i].Y;

        // Front
        glTexCoord2f (0.0f, 0.0f); glVertex3f (X, Y, 0.7f);
        glTexCoord2f (1.0f, 0.0f); glVertex3f (X + 1.0f, Y, 0.7f);
        glTexCoord2f (1.0f, 1.0f); glVertex3f (X + 1.0f, Y - 0.3f, 0.7f);
        glTexCoord2f (0.0f, 1.0f); glVertex3f (X, Y - 0.3f, 0.7f);

#ifdef D6_RENDER_BACKS
        // Back
        glTexCoord2f (0.0f, 0.0f); glVertex3f (X + 1.0f, Y, 0.3f);
        glTexCoord2f (1.0f, 0.0f); glVertex3f (X, Y, 0.3f);
        glTexCoord2f (1.0f, 1.0f); glVertex3f (X, Y - 0.3f, 0.3f);
        glTexCoord2f (0.0f, 1.0f); glVertex3f (X + 1.0f, Y - 0.3f, 0.3f);
#endif
        // Left
        glTexCoord2f (0.0f, 0.0f); glVertex3f (X, Y, 0.3f);
        glTexCoord2f (1.0f, 0.0f); glVertex3f (X, Y, 0.7f);
        glTexCoord2f (1.0f, 1.0f); glVertex3f (X, Y - 0.3f, 0.7f);
        glTexCoord2f (0.0f, 1.0f); glVertex3f (X, Y - 0.3f, 0.3f);

        // Right
        glTexCoord2f (0.0f, 0.0f); glVertex3f (X + 1.0f, Y, 0.7f);
        glTexCoord2f (1.0f, 0.0f); glVertex3f (X + 1.0f, Y, 0.3f);
        glTexCoord2f (1.0f, 1.0f); glVertex3f (X + 1.0f, Y - 0.3f, 0.3f);
        glTexCoord2f (0.0f, 1.0f); glVertex3f (X + 1.0f, Y - 0.3f, 0.7f);

        // Top
        glTexCoord2f (0.0f, 0.0f); glVertex3f (X, Y, 0.3f);
        glTexCoord2f (1.0f, 0.0f); glVertex3f (X + 1.0f, Y, 0.3f);
        glTexCoord2f (1.0f, 1.0f); glVertex3f (X + 1.0f, Y, 0.7f);
        glTexCoord2f (0.0f, 1.0f); glVertex3f (X, Y, 0.7f);

        // Base
        glTexCoord2f (0.0f, 0.0f); glVertex3f (X, Y - 0.3f, 0.7f);
        glTexCoord2f (1.0f, 0.0f); glVertex3f (X + 1.0f, Y - 0.3f, 0.7f);
        glTexCoord2f (1.0f, 1.0f); glVertex3f (X + 1.0f, Y - 0.3f, 0.3f);
        glTexCoord2f (0.0f, 1.0f); glVertex3f (X, Y - 0.3f, 0.3f);
    }

    glEnd ();
}

void ELEV_CheckMan (d6PLSTATE_s *s)
{
    int             i;
    d6ELEVATOR_s    *e;
    float           x, y;

    x = s->X + 0.5f;
    y = s->Y - 1.0f;

    for (i = 0; i < d6Elevators; i++)
    {
        e = &d6Elev[i];

        if (x < e->X || x > e->X + 1.0f)
            continue;

        if (y < e->Y - 0.05f || y > e->Y + 0.05f)
            continue;

        s->Elev = i;
        s->Y = e->Y + 1.0f;
        s->J = 0;
        return;
    }
}

void ELEV_MoveMan (d6PLSTATE_s *s)
{
    d6ELEVATOR_s    *e;

    if (s->Elev >= 0)
    {
        e = &d6Elev[s->Elev];
        s->X += e->AddPos * e->AddX[e->NowSec] * g_app.frame_interval;
        s->Y += e->AddPos * e->AddY[e->NowSec] * g_app.frame_interval;
    }
}

void ELEV_Load (const char *sou, bool mirror)
{
    myFile_s    *f;
    int         l, *e;

    g_app.con->printf (MY_L("APP00026|...Nahravam vytahy - "));
    ELEV_Free ();

    l = MY_FSize (sou) - (12 + 2 * d6World.Level.Size);

    if (l < 1)
        return;

    e = D6_MALLOC (int, l >> 2);

    f = MY_FOpen (sou, 12 + 2 * d6World.Level.Size, "rb", true);
    MY_FRead (e, 4, l >> 2, f);
    MY_FClose (&f);

    g_app.con->printf (MY_L("APP00027|%d vytahu\n"), e[0]);
    l = 1;
    while (e[0]-- > 0)
    {
        ELEV_Add (&e[l], mirror);
        l += 3 + 2 * e[l];
    }

    MY_Free (e);
}
