/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

 /*
	contains the names of the IDs used in profiling code.
 */

typedef struct {
	unsigned id;
	char name[32];
}IdNameStruct;

const IdNameStruct IdName[] = {
	{ PROF_BASH_PREFRAME, 	"Bash_preframe()"	},
	{ PROF_PREFRAME_DO, 	"Preframe_do()"		},
	{ PROF_RENDER_FLOOR, 	"Render_floor()"	},
	{ PROF_RENDER_SPHERES, 	"Render_spheres()"	},
	{ PROF_RENDER_SHADOWS,	"Render_shadows()"	},
	{ PROF_MERGE_BUFFERS, 	"Merge_buffers()"	},
	{ PROF_RENDER_VOXEL,    "Voxel()"		},
	{ PROF_POSTFRAME_DO, 	"Postframe_do()"	},

	{ PROF_ANTIALIAS, 	"antialias"	},
	{ PROF_SHADER1,		"shader-pass1"	},
	{ PROF_SHADER2,		"shader-pass2"	},
	{ PROF_CHECKSUM , 	"checksum"	},
	{ PROF_BLUR_DO  , 	"blur_do"	},
	{ PROF_SHOWFPS  , 	"showfps"	},
	{ PROF_MEMCPY   , 	"memcpy"	},
	{ PROF_SDL_FLIP , 	"sdl_flip"	},
	{ PROF_CONVERTRGB2YUV, 	"convertrgb2yuv"},
	{ PROF_DISPLAYOVERLAY, 	"displayoverlay"},

	{ PROF_PREFRAME_MEMSET,	"preframe_memset"},
	{ PROF_SPHERE_THINGS,	"sphere_things" },
	{ PROF_PHYSICS  ,	"physics"	},
	{ PROF_SORT     ,	"sort"		},
	{ PROF_SHADOWED_CHECK,	"shadowed_check"},
	{ PROF_PROJECT  ,	"project"	},

	{ PROF_YSORT	,	"Ysort"		},
	{ PROF_PASS     ,	"pass"		},
	{ PROF_MESHINIT	,	"mesh_init"	},
	{ PROF_ANTIBUF_INIT,	"antibuff_init"	},

	{ PROF_INIT_PER_ROW,	"init_per_row"	},
	{ PROF_WORK_PER_ROW,	"work_per_row"	},

	{ PROF_MEMSETS  ,	"memsets"	},
	{ PROF_SOLVE3D  ,	"solve3d"	},
	{ PROF_RAYTRACE ,	"raytrace"	},

	{ PROF_ZERO_SBUFFER,	"zero_sbuffer"	},
	{ PROF_SHADOW_PRECALC,	"shadow-precalc"},
	{ PROF_POLY_GEN,	"polygon_gen"	},
	{ PROF_POLY_REARRANGE,	"poly_rearrange"},
	{ PROF_POLY_DISPLAY,    "poly_display"  },
	{ PROF_CONNECT_GRAPH,	"connect_graph" },
	{ PROF_MERGE,		"shadow-merge"	},

	{ PROF_BUFFER_CLEAR,	"buffer clear"	},
	{ PROF_ADDRESS_GENERATE,"addr. generate"},
	{ PROF_INTERPOL_INIT,	"interpol. init"},
	{ PROF_INTERPOLATE,	"interpolation"	},
	{ 0x66604213	,	"hahaha"	} // sentinel, used to mark the end
};

const char *notfound = "NotFound";

const char *getidname(unsigned ID)
{
	int i;
	for (i=0;IdName[i].id != 0x66604213; i++) if (IdName[i].id == ID)
		return IdName[i].name;
	return notfound;
}
