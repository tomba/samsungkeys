/*
 *  This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* http://git.kernel.org/?p=linux/kernel/git/torvalds/linux.git;a=blob;f=drivers/platform/x86/samsung-laptop.c */

#include <stdio.h>
#include <Windows.h>

//#define SABI_DBG

#define SABI_CMD_KBD_BL 0x78

#define SEG_LEN 0x10000

#pragma pack (1)
struct sabi_data
{
	UINT32 d0;
	UINT32 d1;
	UINT16 d2;
	UINT8  d3;
};

struct sabi_msg
{
	UINT16 main_func;

	UINT16 cmd;

	UINT8 pad;

	struct sabi_data data;
};

struct sabi_msg_in
{
	UINT8 hdr[10];

	struct sabi_msg msg;
};
#pragma pack ()

static void print_sabi_data(const struct sabi_data *sdata)
{
	printf("SABI 0x%08x, 0x%08x, 0x%04x, 0x%02x\n", sdata->d0, sdata->d1, sdata->d2, sdata->d3);
}

static HANDLE hFile;
static UINT8 sabi_hdr[10];

static bool send_sabi_cmd(UINT16 cmd, const struct sabi_data *in, struct sabi_data *out)
{
	struct sabi_msg_in sd_in = { 0 };

	memcpy(sd_in.hdr, sabi_hdr, sizeof(sabi_hdr));
	sd_in.msg.main_func = 0x5843;
	sd_in.msg.cmd = cmd;
	sd_in.msg.data = *in;

#ifdef SABI_DBG
	printf("sending:\n");

	/*
	for (size_t i = 0; i < sizeof(sd_in); ++i)
	printf("%02X ", ((UINT8 *)&sd_in)[i]);
	printf("\n");
	*/

	print_sabi_data(&sd_in.msg.data);
#endif

	struct sabi_msg sd_out;
	DWORD len;

	BOOL r = DeviceIoControl(hFile, 0x9C412004, &sd_in, sizeof(sd_in), &sd_out, sizeof(sd_out), &len, NULL);
	if (r == false)
	{
		printf("failed to send cmd\n");
		return false;
	}

	if (len != sizeof(sd_out))
	{
		printf("illegal return len\n");
		return false;
	}

#ifdef SABI_DBG
	printf("received:\n");

	/*
	for (size_t i = 0; i < sizeof(sd_out); ++i)
	printf("%02X ", ((UINT8 *)&sd_out)[i]);
	printf("\n");
	*/

	print_sabi_data(&sd_out.data);
#endif

	*out = sd_out.data;

	return true;
}

static bool read_sabi_hdr()
{
	unsigned char *buf;
	DWORD len;

	buf = (unsigned char *)malloc(SEG_LEN);

	BOOL r = DeviceIoControl(hFile, 0x9C412000, NULL, 0, buf, SEG_LEN, &len, NULL);
	if (r == false)
	{
		printf("failed to read bios block\n");
		goto err1;
	}

	const char *str = "SwSmi@";
	unsigned char *b = NULL;

	for (unsigned int i = 0; i < SEG_LEN - strlen(str); ++i)
	{
		if (strncmp((char *)buf + i, str, strlen(str)) == 0)
		{
			b = buf + i + strlen(str);
			break;
		}
	}

	if (b == NULL)
	{
		printf("failed to find the block\n");
		goto err1;
	}

	memcpy(&sabi_hdr, b, sizeof(sabi_hdr));

	free(buf);
	return true;

err1:
	free(buf);
	return false;
}

static bool get_kbd_bl(unsigned int *level)
{
	struct sabi_data sd_in = { 0 };
	struct sabi_data sd_out = { 0 };

	sd_in.d0 = 0x81; // read keyboard backlight

	bool r = send_sabi_cmd(SABI_CMD_KBD_BL, &sd_in, &sd_out);
	if (!r)
		return r;

	*level = (sd_out.d0 & 0xff);

	return true;
}

static bool set_kbd_bl(unsigned int level)
{
	struct sabi_data sd_in = { 0 };
	struct sabi_data sd_out = { 0 };

	if (level > 8)
	{
		printf("too high level\n");
		return false;
	}

	sd_in.d0 = 0x82 | (level << 8); // set keyboard backlight

	bool r = send_sabi_cmd(SABI_CMD_KBD_BL, &sd_in, &sd_out);
	if (!r)
		return r;

	return true;
}

void sabi_kbd_bl_inc()
{
	unsigned int val;

	get_kbd_bl(&val);

	if (val == 8)
		return;

	set_kbd_bl(val + 1);
}

void sabi_kbd_bl_dec()
{
	unsigned int val;

	get_kbd_bl(&val);

	if (val == 0)
		return;

	set_kbd_bl(val - 1);
}

bool sabi_init()
{
	hFile = CreateFileW(L"\\\\.\\SABI\\SabiDrv.log", GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("driver open failed\n");
		return false;
	}

	bool r;

	r = read_sabi_hdr();

	if (r == false)
	{
		printf("header read failed\n");
		CloseHandle(hFile);
		hFile = NULL;
		return false;
	}

	return true;
}

void sabi_uninit()
{
	if (hFile != NULL)
	{
		CloseHandle(hFile);
		hFile = NULL;
	}
}
