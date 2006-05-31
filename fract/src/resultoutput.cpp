/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *   Handles official .result file creation and security checking          *
 ***************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "MyGlobal.h"
#include "aes.h"
#include "cmdline.h"
#include "cpuid.h"
#include "gfx.h"
#include "profile.h"
#include "resultoutput.h"

const char *allowed_args[] = { "--official", "--cpus", "--no-thread", "" };
extern int saved_cpuspd;

bool are_args_ok(void)
{
	bool args_ok = true;
	for (ArgumentIterator arg; arg.ok(); ++arg) {
		bool this_ok = false;
		for (int i = 0; allowed_args[i][0]; i++) {
			if (!strcmp(allowed_args[i], arg.value())) {
				this_ok = true;
				break;
			}
		}
		if (!this_ok) {
			args_ok = false;
			break;
		}
	}
	return args_ok;
}

static char *mdfilelist[] = {
	"data/3pyramid.obj",
	"data/benchmark.fsv",
	"data/oldbench.fsv",
	"data/2.bmp",
	"data/truncated_cube.obj",
	"data/truncated_cube.mtl",
	"data/zar-texture.bmp",
	""
};

unsigned file_hash_ok[4] = {0x0E8D9991, 0x98EEE322, 0x1EC1EE09, 0x859B7967};
bool MD5Check(void)
{
	MD5Hasher hash;
	for (int i = 0; mdfilelist[i][0]; i++) {
		hash.add_file(mdfilelist[i]);
	}
	unsigned file_hash[4];
	hash.result(file_hash);
	for (int i = 0; i < 4; i++) {
		if (file_hash[i] != file_hash_ok[i]) {
#ifdef MD5_CHECK
			return false;
#else
			printf("Hash values are inconsistent:\n");
			printf("_OK_:"); 
			for (int i = 0; i < 4; i++)
				printf("%08X ", file_hash_ok[i]);
			printf(" vs.\n");
			printf("This:"); 
			for (int i = 0; i < 4; i++)
				printf("%08X ", file_hash[i]);
			printf("\n");
#endif
			break;
		}
	}
	return true;
}

/*
Implementation specific things:
*/

const char *get_OS(void)
{
#if defined __linux__ || defined linux
	return "Linux";
#else
	return "Windows";
#endif
}

void get_compiler_version(char *out)
{
#ifdef _MSC_VER
	strcpy(out, "MSVC ");
	char blah[20];
	sprintf(blah, "ver. %.1f", _MSC_VER / 100.0f-6.0f);
	strcat(out, blah);
#else
#ifdef __GNUC__
	strcpy(out, "gcc ");
	char blaf[20];
	sprintf(blaf, "%d.%d", __GNUC__, __GNUC_MINOR__);
	strcat(out, blaf);
#else 
	strcpy(out, "Other Compiler");
#endif //__GNUC__
#endif // _MSC_VER
}


void generate_result_file(FPSWatch & watch)
{
	if (!are_args_ok()) {
		printf("Commandline arguments violate the requirements for official test!\n");
		printf("Sorry, you can't run fract for official result with these options!\n");
		return;
	}
	if (!MD5Check()) {
		printf("The program's data files seem to be modified or corrupted.\n");
		printf("Please, download a fresh copy from http://fbench.com\n");
		return;
	}
	char username[101], cputype[101], country[101], chipset[101];
	char comment[MAX_COMMENT + 2];
	FILE *f = fopen(USER_INFO_FILE, "rt");
	if (!f) {
		printf("User info file `%s' not found; make sure you ran fract from\n", USER_INFO_FILE);
		printf("Fract_launcher\n");
		return;
	}
	if (!fgets(username, 100, f)) {
		printf("Failed to read your username from file `%s'!\n", USER_INFO_FILE);
		fclose(f);
		return;
	}
	if (strlen(username) > 31) {
		printf("Your username is too long; it must be less than 32 characters\n");
		fclose(f);
		return;
	}
	if (!fgets(cputype, 100, f)) {
		printf("Failed to read your CPU type from file `%s'!\n", USER_INFO_FILE);
		fclose(f);
		return;
	}
	if (strlen(cputype) > 31) {
		printf("Your CPU Type name is too long; it must be less than 32 characters\n");
		fclose(f);
		return;
	}
	if (!fgets(country, 100, f)) {
		printf("Failed to read your country from file `%s'!\n", USER_INFO_FILE);
		fclose(f);
		return;
	}
	if (strlen(country) > 15) {
		printf("Your country name is too long; it must be less than 16 characters\n");
		fclose(f);
		return;
	}
	if (!fgets(chipset, 100, f)) {
		printf("Failed to read your chipset from file `%s'!\n", USER_INFO_FILE);
		fclose(f);
		return;
	}
	if (strlen(chipset) > 15) {
		printf("Your chipset name is too long; it must be less than 16 characters\n");
		fclose(f);
		return;
	}
	fclose(f);
	f = fopen(USER_COMMENT_FILE, "rb");
	if(!f) {
		printf("User comment file `%s' not found; make sure you ran fract\n", USER_COMMENT_FILE);
		printf("from Fract_launcher\n");
		return;
	}
	size_t rd = fread(comment, 1, MAX_COMMENT+1, f);
	fclose(f);
	if (rd > MAX_COMMENT - 1) {
		printf("Your comment is too long. It must be less than %d characters\n", MAX_COMMENT);
		return;
	}
	comment[rd] = 0;
	srand(watch.total_data());
	result_file a;
	unsigned char *x = (unsigned char*) &a;
	for (unsigned i = 0; i < sizeof(a); i++) {
		x[i] = 0xff & (rand() % 541);
	}

	//strcpy(a.version, Fract_Version);
	sprintf(a.version, "%s/%s [%s]", Fract_Version, Mod_Instruction_Set, __DATE__);
	strcpy(a.OS, get_OS());
	get_compiler_version(a.compiler);
	a.cpu_count = SysInfo.cpu_count();
	strcpy(a.cpu_type, cputype);
	if (saved_cpuspd == -1) {
		saved_cpuspd = GetCPUSpeed();
	}
	a.cpu_mhz = saved_cpuspd;
	a.res_x = xres();
	a.res_y = yres();
	a.overall_fps = (float) (watch.total_data() / watch.total());
	for (int i = 0; i < watch.size(); i++) {
		a.scene[i] = (float) (watch.get_data(i)/ watch[i]);
	}
	for (int i = watch.size(); i < 5; i++)
		a.scene[i] = 0.0f;
	strcpy(a.username, username);
	strcpy(a.country, country);
	strcpy(a.comment, comment);
	strcpy(a.chipset, chipset);
	float saved_fps = a.overall_fps;
	a.calculate_md5_sum((void*)a.md5sum);
	AESEncrypt(&a, sizeof(a));
	char fn[64];
	sprintf(fn, "%s_%.2f", username, saved_fps);
	for (unsigned i = 0; i < strlen(fn); i++)
		if (!isalnum(fn[i])) fn[i] = '_';
	strcat(fn, ".result");
	f = fopen(fn, "wb");
	if (!f) {
		printf("Result output file `%s' failed to open for unknown reason\n", fn);
		printf("Your result hasn't been saved\n");
		return;
	}
	fwrite(&a, sizeof(a), 1, f);
	fclose(f);
}

int query_integrity(void)
{
	return MD5Check()?0:1;
}
