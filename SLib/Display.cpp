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

#include <stdio.h>
#include <Windows.h>
#include <powersetting.h>

static bool get_brightness(GUID *pwrGUID, UINT32 *val)
{
	DWORD r, size;
	SYSTEM_POWER_STATUS pwr_status;

	GetSystemPowerStatus(&pwr_status);

	bool dc = pwr_status.ACLineStatus == 0;

	size = sizeof(*val);

	if (dc)
		r = PowerReadDCValue(NULL, pwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, NULL,
		(unsigned char *)val, &size);
	else
		r = PowerReadACValue(NULL, pwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, NULL,
		(unsigned char *)val, &size);

	if (r != ERROR_SUCCESS)
	{
		printf("PowerReadDC/ACValue failed: %u\n", r);
		return false;
	}

	return true;
}

static bool set_brightness(GUID *pwrGUID, UINT32 val)
{
	DWORD r, size;
	SYSTEM_POWER_STATUS pwr_status;

	GetSystemPowerStatus(&pwr_status);

	bool dc = pwr_status.ACLineStatus == 0;

	size = sizeof(val);

	if (dc)
		r = PowerWriteDCValueIndex(NULL, pwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, val);
	else
		r = PowerWriteACValueIndex(NULL, pwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, val);

	if (r != ERROR_SUCCESS)
	{
		printf("PowerWriteDC/ACValueIndex failed: %u\n", r);
		return false;
	}

	r = PowerSetActiveScheme(NULL, pwrGUID);
	if (r != ERROR_SUCCESS)
	{
		printf("PowerSetActiveScheme failed: %u\n", r);
		return false;
	}

	return true;
}

static void display_bl_set()
{
	GUID *pPwrGUID;
	DWORD r;
	DWORD size;
	UINT32 v;

	r = PowerGetActiveScheme(NULL, &pPwrGUID);
	if (r != ERROR_SUCCESS)
	{
		printf("PowerGetActiveScheme failed: %u\n", r);
		return;
	}

	size = sizeof(v);

	r = PowerReadDCValue(NULL, pPwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, NULL,
		(unsigned char *)&v, &size);
	if (r != ERROR_SUCCESS)
	{
		printf("PowerReadDCValue failed: %u\n", r);
		return;
	}

	v = 0;

	r = PowerWriteDCValueIndex(NULL, pPwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, v);
	if (r != ERROR_SUCCESS)
	{
		printf("PowerWriteDCValueIndex failed: %u\n", r);
		return;
	}

	r = PowerSetActiveScheme(NULL, pPwrGUID);
	if (r != ERROR_SUCCESS)
	{
		printf("PowerSetActiveScheme failed: %u\n", r);
		return;
	}
}

void display_bl_inc()
{
	GUID *pwrGUID;
	DWORD r;

	r = PowerGetActiveScheme(NULL, &pwrGUID);
	if (r != ERROR_SUCCESS)
	{
		printf("PowerGetActiveScheme failed: %u\n", r);
		return;
	}

	UINT32 val;

	get_brightness(pwrGUID, &val);

	if (val == 100)
		return;

	if (val > 90)
		val = 100;
	else
		val += 10;

	set_brightness(pwrGUID, val);
}

void display_bl_dec()
{
	GUID *pwrGUID;
	DWORD r;

	r = PowerGetActiveScheme(NULL, &pwrGUID);
	if (r != ERROR_SUCCESS)
	{
		printf("PowerGetActiveScheme failed: %u\n", r);
		return;
	}

	UINT32 val;

	get_brightness(pwrGUID, &val);

	if (val == 0)
		return;

	if (val < 10)
		val = 0;
	else
		val -= 10;

	set_brightness(pwrGUID, val);
}
