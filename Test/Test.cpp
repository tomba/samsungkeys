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

#include <Windows.h>
#include <stdio.h>
#include "../SLib/SLib.h"

static HHOOK m_hookId;

static LRESULT CALLBACK key_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0 || wParam != WM_KEYDOWN)
		return CallNextHookEx(m_hookId, nCode, wParam, lParam);

	KBDLLHOOKSTRUCT *d = (KBDLLHOOKSTRUCT *)lParam;

	if (d->vkCode == 255 && d->flags == 1)
	{
		switch (d->scanCode)
		{
		case 8:
			printf("brightness +\n");
			display_bl_inc();
			break;

		case 9:
			printf("brightness -\n");
			display_bl_dec();
			break;

		case 22:
			printf("kb brightness +\n");
			sabi_kbd_bl_inc();
			break;

		case 23:
			printf("kb brightness -\n");
			sabi_kbd_bl_dec();
			break;
		}
	}

	return CallNextHookEx(m_hookId, nCode, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	AllocConsole();
	freopen("CONOUT$", "wb", stdout);

	sabi_init();

	m_hookId = SetWindowsHookEx(WH_KEYBOARD_LL, &key_hook, hInstance, 0);

	if (m_hookId == NULL)
	{
		printf("failed to register keyboard hook\n");
		return -1;
	}

	MSG Msg;
	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		DispatchMessage(&Msg);
	}

	UnhookWindowsHookEx(m_hookId);

	sabi_uninit();

	return 0;
}
