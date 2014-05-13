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

#include <time.h>
#include <stdlib.h>
#include <SDL/SDL_mixer.h>
#include "project.h"
#include "Weapon.h"
#include "Util.h"
#include "ElevatorList.h"
#include "Math.h"
#include "Menu.h"

namespace Duel6
{
	static bool d6BcgLoaded = false;

	struct VideoMode
	{
		Int32 bpp;
		Int32 aa;
		Int32 width;
		Int32 height;
	};

	static VideoMode d6VideoMode = { 
		D6_CL_BPP, 
		D6_CL_AA, 
		D6_CL_WIDTH, 
		D6_CL_HEIGHT 
	};

	/*
	==================================================
	Video init
	==================================================
	*/
	void SET_InitVideo()
	{
		g_app.con->printf(MY_L("APP00055|\n===Nastavuji OpenGL okno===\n"));

		// Get current video mode
		const SDL_VideoInfo* cur_vid = SDL_GetVideoInfo();
		if (!d6VideoMode.width)
		{
			d6VideoMode.width = cur_vid->current_w;
		}
		if (!d6VideoMode.height)
		{
			d6VideoMode.height = cur_vid->current_h;
		}
		if (!d6VideoMode.bpp)
		{
			d6VideoMode.bpp = cur_vid->vfmt->BitsPerPixel;
		}

		// Set graphics mode
#ifdef _DEBUG
		VID_SetMode (800, 600, d6VideoMode.bpp, d6VideoMode.aa, false);  // Running fullscren makes switching to debugger problematic with SDL (focus is captured)
#else
		VID_SetMode(d6ConVar.vid_width, d6ConVar.vid_height, d6ConVar.bpp, d6ConVar.aa, true);
#endif

		g_vid.gl_fov = 45.0f;
		g_vid.gl_nearclip = 0.1f;
		g_vid.gl_farclip = 100.0f;

		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		glCullFace(GL_FRONT);
		glAlphaFunc(GL_GEQUAL, 1);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		RENDER_SetGLMode(D6_GL_ORTHO);
	}

	void SET_LoadBackground(int n)
	{
		if (d6BcgLoaded)
		{
			glDeleteTextures(1, &d6BackgroundTexture);
		}

		g_app.con->printf(MY_L("APP00059|...Nahravam pozadi (%s, %d)\n"), D6_FILE_BACK, n);
		d6BackgroundTexture = Util::loadKH3Texture(D6_FILE_BACK, n, false);
		d6BcgLoaded = true;
	}

	/*
	==================================================
	Pokud neexistuji soubory config.txt a duel6.dat tak
	je vytvori - pro snadnejsi update dodavany bez techto
	souboru
	==================================================
	*/
	void SET_InitUserFiles()
	{
		char        def_file[50];
		const char *user_file[3] = { D6_FILE_CONFIG, D6_FILE_PHIST, D6_FILE_SKIN };
		int         i;
		mySIZE      len;
		myBYTE      *ptr;
		myFile_s    *f;

		for (i = 0; i < 3; i++)
		{
			if (MY_FSize(user_file[i]) < 1)
			{
				sprintf(def_file, "%s.def", user_file[i]);
				len = MY_FSize(def_file);
				ptr = (myBYTE *)MY_Alloc(len);
				MY_FLoad(def_file, ptr);
				f = MY_FOpen(user_file[i], 0, "wb", true);
				MY_FWrite(ptr, 1, len, f);
				MY_FClose(&f);
				MY_Free(ptr);
			}
		}
	}

	/*
	==================================================
	Set language - console command
	==================================================
	*/
	void SET_Language(con_c *con)
	{
		if (con->argc() == 2)
		{
			if (!strcmp(con->argv(1), "lang/czech.lang"))
				MY_LangFree();
			MY_LangLoad(con->argv(1));
		}
		else
			con->printf("%s: %s\n", MY_L("APP00070|Jazyk"), MY_L("APP00071|cestina"));
	}

	/*
	==================================================
	Nastaveni poctu kol pres konzoli
	==================================================
	*/
	void SET_MaxRounds(con_c *con)
	{
		if (con->argc() == 2)
			d6MaxRounds = atoi(con->argv(1));
		else
			con->printf("Odehranych kol: %d | Max pocet kol: %d\n", d6PlayedRounds, d6MaxRounds);
	}
	
	/*
	==================================================
	Nastaveni volume hudby pres konzolu
	==================================================
	*/
	void SET_Volume(con_c *con)
	{
		if (con->argc() == 2)
			SOUND_Volume(atoi(con->argv(1)));
	}

	void SET_ToggleRenderMode(con_c *con)
	{
		d6Wireframe = !d6Wireframe;
		if (d6Wireframe)
			con->printf(MY_L("APP00020|Vykreslovaci mod prepnut na dratovy\n"));
		else
			con->printf(MY_L("APP00021|Vykreslovaci mod prepnut na solidni\n"));
	}

	void SET_ToggleShowFps(con_c *con)
	{
		d6ShowFps = !d6ShowFps;
		if (d6ShowFps)
			con->printf(MY_L("APP00022|Ukazatel fps zapnut\n"));
		else
			con->printf(MY_L("APP00023|Ukazatel fps vypnut\n"));
	}

	/*
	==================================================
	Zapnuti/vypnuti hudby v menu
	==================================================
	*/
	void SET_MusicOnOff(con_c *con)
	{
		if (con->argc() == 2)
		{
			if (!strcmp(con->argv(1), "on"))
			{
				d6PlayMusic = true;
				if (d6InMenu)
					SOUND_StartMusic(0, false);
			}
			if (!strcmp(con->argv(1), "off"))
			{
				d6PlayMusic = false;
				if (d6InMenu)
					SOUND_StopMusic();
			}
		}
	}

	/*
	==================================================
	Vyhledani nove pripojenych joypadu
	==================================================
	*/
	void SET_JoyScan(con_c *con)
	{
		if (d6InMenu)
		{
			MENU_JoyRescan();
		}
	}

	/*
	==================================================
	Nastaveni skinu
	==================================================
	*/
	void SET_LoadSkin(con_c *con)
	{
		int     i, pl, pos, num, exp16, c;

		if (con->argc() == 11)
		{
			pl = atoi(con->argv(1));
			if (pl >= 0 && pl < (int)d6PlayerColors.size())
			{
				for (i = 0; i < 9; i++)
				{
					pos = strlen(con->argv(i + 2)) - 1;
					num = 0;
					exp16 = 1;
					while (pos >= 0)
					{
						c = con->argv(i + 2)[pos];
						if (c >= '0' && c <= '9')
							num += (c - '0') * exp16;
						if (c >= 'a' && c <= 'f')
							num += (c - 'a' + 10) * exp16;
						pos--;
						exp16 *= 16;
					}

					Color color((num & 0xff0000) >> 16, (num & 0xff00) >> 8, num & 0xff);
					d6PlayerColors[pl].set((PlayerSkinColors::BodyPart)i, color);
				}

				con->printf("Skin %d: OK\n", pl);
			}
		}
		if (con->argc() == 2)
		{
			pl = atoi(con->argv(1));
			if (pl >= 0 && pl < (int)d6PlayerColors.size())
			{
				con->printf("Skin %d: ", pl);
				for (i = 0; i < 9; i++)
				{
					const Color& color = d6PlayerColors[pl].get((PlayerSkinColors::BodyPart)i);
					con->printf("%02x%02x%02x ", color.getRed(), color.getGreen(), color.getBlue());
				}
				con->printf("\n");
			}
		}
	}

	/*
	==================================================
	Povoleni/zakazani zbrane
	==================================================
	*/
	void SET_EnableWeapon(con_c *con)
	{
		int gn;

		if (con->argc() == 3)
		{
			gn = atoi(con->argv(1));
			if (gn >= 0 && gn < D6_WEAPONS)
			{
				d6WpnDef[gn].enabled = !strcmp(con->argv(2), "true");
				if (d6WpnDef[gn].enabled)
					con->printf("\t%02d. %-13s %s\n", gn, MY_L(d6WpnDef[gn].Name), MY_L("APP00113|povoleno"));
				else
					con->printf("\t%02d. %-13s %s\n", gn, MY_L(d6WpnDef[gn].Name), MY_L("APP00114|zakazano"));
			}
		}
		else
		{
			for (gn = 0; gn < D6_WEAPONS; gn++)
			{
				if (d6WpnDef[gn].enabled)
					con->printf("\t%02d. %-13s %s\n", gn, MY_L(d6WpnDef[gn].Name), MY_L("APP00113|povoleno"));
				else
					con->printf("\t%02d. %-13s %s\n", gn, MY_L(d6WpnDef[gn].Name), MY_L("APP00114|zakazano"));
			}
		}
	}

	/*
	==================================================
	Nastaveni rozsahu poctu naboju po startu hry
	==================================================
	*/
	void SET_AmmoRange(con_c *con)
	{
		if (con->argc() == 3)
		{
			int min = atoi(con->argv(1)), max = atoi(con->argv(2));

			if (min <= max && min >= 0)
			{
				d6AmmoRangeMin = min;
				d6AmmoRangeMax = max;
			}
		}
		else if (con->argc() == 1)
		{
			con->printf("\tmin = %d\n\tmax = %d\n", d6AmmoRangeMin, d6AmmoRangeMax);
		}
		else
		{
			con->printf("%s: %s [min max]\n", con->argv(0), con->argv(0));
		}
	}

	/** OpenGL info */
	void SET_OpenGLInfo(con_c *con)
	{
		const char *e;
		char *txp = NULL, *tx, tx2[200];
		int len, i;

		con->printf(MY_L("APP00075|\n===OpenGL info===\n"));
		con->printf(MY_L("APP00076|Vyrobce    : %s\n"), glGetString(GL_VENDOR));
		con->printf(MY_L("APP00077|Renderer   : %s\n"), glGetString(GL_RENDERER));
		con->printf(MY_L("APP00078|Verze      : %s\n"), glGetString(GL_VERSION));
		con->printf(MY_L("APP00079|Extenze    :\n"));

		e = (const char *)glGetString(GL_EXTENSIONS);

		if (strlen(e) < 2)
		{
			con->printf(MY_L("APP00080|...Zadne podporovane extenze\n"));
		}
		else
		{
			txp = (char *)MY_Alloc(strlen(e) + 1);
			strcpy(txp, e);
			tx = txp;

			while (*tx)
			{
				while (*tx == ' ')
					tx++;

				if (*tx == 0)
					break;

				len = 0;

				while (tx[len] != ' ' && tx[len] != 0)
					len++;

				for (i = 0; i < len; i++, tx++)
					tx2[i] = *tx;

				tx2[len] = 0;

				con->printf("...%s\n", tx2);
			}

			MY_Free(txp);
		}

		con->printf("\n");
	}

	/*
	==================================================
	Main init
	==================================================
	*/
	void P_Init()
	{
		const SDL_version   *sdl_ver;
		const char          *ver_str = MY_L("APP00072|verze");

		// Print application info
		g_app.con->printf(MY_L("APP00073|\n===Informace o aplikaci===\n"));
		g_app.con->printf("%s %s: %s\n", APP_NAME, ver_str, APP_VERSION);
		g_app.con->printf("Mylib %s: %s\n", ver_str, MYLIB_VERSION);
		sdl_ver = SDL_Linked_Version();
		g_app.con->printf("SDL %s: %d.%d.%d\n", ver_str, sdl_ver->major, sdl_ver->minor, sdl_ver->patch);
		sdl_ver = Mix_Linked_Version();
		g_app.con->printf("SDL_mixer %s: %d.%d.%d\n", ver_str, sdl_ver->major, sdl_ver->minor, sdl_ver->patch);
		g_app.con->printf(MY_L("APP00074|Jazyk: cestina\n"));

		// Set some console functions
		g_app.con->setlast(15);
		g_app.con->regcmd(&SET_ToggleRenderMode, "switch_render_mode");
		g_app.con->regcmd(&SET_ToggleShowFps, "show_fps");
		g_app.con->regcmd(&SET_OpenGLInfo, "gl_info");
		g_app.con->regcmd(&SET_Language, "lang");
		g_app.con->regcmd(&SET_Volume, "volume");
		g_app.con->regcmd(&SET_MaxRounds, "rounds");
		g_app.con->regcmd(&SET_MusicOnOff, "music");
		g_app.con->regcmd(&SET_JoyScan, "joy_scan");
		g_app.con->regcmd(&SET_LoadSkin, "skin");
		g_app.con->regcmd(&SET_EnableWeapon, "gun");
		g_app.con->regcmd(&SET_AmmoRange, "start_ammo_range");
		g_app.con->regvar(&g_app.fps, "g_fps", CON_F_RONLY, CON_VAR_FLOAT);
		g_app.con->regvar(&d6VideoMode.aa, "g_aa", CON_F_NONE, CON_VAR_INT);
		g_app.con->regvar(&d6VideoMode.bpp, "g_bpp", CON_F_NONE, CON_VAR_INT);
		g_app.con->regvar(&d6MaxRounds, "rounds", CON_F_NONE, CON_VAR_INT);
		g_app.con->regvar(&d6VideoMode.width, "g_cl_width", CON_F_NONE, CON_VAR_INT);
		g_app.con->regvar(&d6VideoMode.height, "g_cl_height", CON_F_NONE, CON_VAR_INT);

		srand((unsigned)time(NULL));

		MY_FLoadBlock(D6_FILE_COS, 0, -1, (void *)d6Cos);

		SOUND_Init(20, 30, 1);

		SET_InitUserFiles();

		// Read config file
		g_app.con->printf("\n===Config===\n");
		g_app.con->exec("exec data/config.txt");
		// Init player skins
		g_app.con->printf("\n====Skin====\n");
		g_app.con->exec("exec data/skin.txt");

		SET_InitVideo();
		d6World.init(D6_FILE_ART, D6_FILE_ANM);
		WPN_Init(D6_FILE_WEAPON);
		EXPL_Load(D6_FILE_EXPLODE);
		ELEV_Init();
		FIRE_Init();

		MENU_Init();
		MENU_Start();
	}

	void P_DeInit()
	{
		SOUND_DeInit();

		d6World.freeTextures();
		WPN_FreeTextures();
		MENU_Free();
		EXPL_Free();

		WPN_DeInit();
	}
}