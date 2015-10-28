// Copyright © Camilla Löwy Berglund <elmindreda@glfw.org>
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
//    distribution.

#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char* utf16_to_utf8(const WCHAR* source)
{
    char* target;
    int length;

    length = WideCharToMultiByte(CP_UTF8, 0, source, -1, NULL, 0, NULL, NULL);
    if (!length)
        return NULL;

    target = calloc(length, sizeof(char));

    if (!WideCharToMultiByte(CP_UTF8, 0, source, -1, target, length, NULL, NULL))
    {
        free(target);
        return NULL;
    }

    return target;
}

static void indent(int depth)
{
    depth *= 4;
    while (depth--)
        putchar(' ');
}

static void print_modes(int depth, const DISPLAY_DEVICEW* device)
{
    int modeIndex;

    for (modeIndex = 0;  ;  modeIndex++)
    {
        DEVMODEW dm;

        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);

        if (!EnumDisplaySettingsW(device->DeviceName, modeIndex, &dm))
            break;

        indent(depth);
        printf("Mode");

        if (dm.dmFields & DM_POSITION)
            printf(" Position: %li,%li", dm.dmPosition.x, dm.dmPosition.y);
        if (dm.dmFields & DM_DISPLAYORIENTATION)
        {
            printf(" Orientation: ");
            if (dm.dmDisplayOrientation == DMDO_DEFAULT)
                printf("DEFAULT");
            if (dm.dmDisplayOrientation == DMDO_90)
                printf("90");
            if (dm.dmDisplayOrientation == DMDO_180)
                printf("180");
            if (dm.dmDisplayOrientation == DMDO_270)
                printf("270");
        }
        if (dm.dmFields & DM_DISPLAYFIXEDOUTPUT)
        {
            printf(" FixedOutput: ");
            if (dm.dmDisplayFixedOutput == DMDFO_DEFAULT)
                printf("DEFAULT");
            if (dm.dmDisplayFixedOutput == DMDFO_CENTER)
                printf("CENTER");
            if (dm.dmDisplayFixedOutput == DMDFO_STRETCH)
                printf("STRETCH");
        }
        if (dm.dmFields & DM_LOGPIXELS)
            printf(" LogPixels: %u", dm.dmLogPixels);
        if (dm.dmFields & DM_BITSPERPEL)
            printf(" BitsPerPel: %u", dm.dmBitsPerPel);
        if (dm.dmFields & DM_PELSWIDTH)
            printf(" PelsWidth: %u", dm.dmPelsWidth);
        if (dm.dmFields & DM_PELSHEIGHT)
            printf(" PelsHeight: %u", dm.dmPelsHeight);
        if (dm.dmFields & DM_DISPLAYFREQUENCY)
            printf(" DisplayFrequency: %u", dm.dmDisplayFrequency);

        if (device->StateFlags & DISPLAY_DEVICE_MODESPRUNED)
        {
            printf(" Test: ");

            const LONG result = ChangeDisplaySettingsExW(device->DeviceName,
                                                         &dm,
                                                         NULL,
                                                         CDS_TEST,
                                                         NULL);
            if (result == DISP_CHANGE_SUCCESSFUL)
                printf("SUCCESSFUL");
            if (result == DISP_CHANGE_BADDUALVIEW)
                printf("BADDUALVIEW");
            if (result == DISP_CHANGE_BADFLAGS)
                printf("BADFLAGS");
            if (result == DISP_CHANGE_BADMODE)
                printf("BADMODE");
            if (result == DISP_CHANGE_BADPARAM)
                printf("BADPARAM");
            if (result == DISP_CHANGE_FAILED)
                printf("FAILED");
            if (result == DISP_CHANGE_NOTUPDATED)
                printf("NOTUPDATED");
            if (result == DISP_CHANGE_RESTART)
                printf("RESTART");
        }

        putchar('\n');
    }
}

static void print_device(int depth, const DISPLAY_DEVICEW* device)
{
    HDC dc;
    char* name = utf16_to_utf8(device->DeviceName);
    char* string = utf16_to_utf8(device->DeviceString);

    indent(depth);
    printf("Device Name: \"%s\" String: \"%s\"", name, string);

    if (device->StateFlags)
    {
        printf(" State:");

        if (device->StateFlags & DISPLAY_DEVICE_ACTIVE)
            printf(" ACTIVE");
        if (device->StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
            printf(" MIRRORING_DRIVER");
        if (device->StateFlags & DISPLAY_DEVICE_MODESPRUNED)
            printf(" MODESPRUNED");
        if (device->StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            printf(" PRIMARY_DEVICE");
        if (device->StateFlags & DISPLAY_DEVICE_REMOVABLE)
            printf(" REMOVABLE");
        if (device->StateFlags & DISPLAY_DEVICE_VGA_COMPATIBLE)
            printf(" VGA_COMPATIBLE");
    }

    dc = CreateDCW(L"DISPLAY", device->DeviceName, NULL, NULL);
    printf(" HorzSize: %u VertSize: %u",
           GetDeviceCaps(dc, HORZSIZE),
           GetDeviceCaps(dc, VERTSIZE));
    DeleteDC(dc);

    putchar('\n');

    free(name);
    free(string);
}

static void print_devices(int depth)
{
    DWORD adapterIndex, displayIndex;

    for (adapterIndex = 0;  ;  adapterIndex++)
    {
        DISPLAY_DEVICEW adapter;

        ZeroMemory(&adapter, sizeof(adapter));
        adapter.cb = sizeof(adapter);

        if (!EnumDisplayDevicesW(NULL, adapterIndex, &adapter, 0))
            break;

        print_device(depth, &adapter);

        for (displayIndex = 0;  ;  displayIndex++)
        {
            DISPLAY_DEVICEW display;

            ZeroMemory(&display, sizeof(display));
            display.cb = sizeof(display);

            if (!EnumDisplayDevicesW(adapter.DeviceName, displayIndex, &display, 0))
                break;

            print_device(depth + 1, &display);
        }

        print_modes(depth + 1, &adapter);
    }
}

static BOOL CALLBACK monitor_callback(HMONITOR monitor,
                                      HDC dc, LPRECT rect, LPARAM data)
{
    MONITORINFOEXW mi;
    char* device;

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);

    if (!GetMonitorInfoW(monitor, (MONITORINFO*) &mi))
        return TRUE;

    device = utf16_to_utf8(mi.szDevice);

    indent((int) data);
    printf("Monitor Device: \"%s\" Rect: %i,%i,%i,%i",
           device, rect->left, rect->top, rect->right, rect->bottom);

    if (mi.dwFlags & MONITORINFOF_PRIMARY)
        printf(" PRIMARY");

    putchar('\n');

    free(device);
    return TRUE;
}

static void print_monitors(int depth)
{
    EnumDisplayMonitors(NULL, NULL, monitor_callback, depth);
}

int main(int argc, char** argv)
{
    printf("Desktop MonitorCount: %i VirtualScreen: %i,%i,%i,%i RemoteSession: %i\n",
           GetSystemMetrics(SM_CMONITORS),
           GetSystemMetrics(SM_XVIRTUALSCREEN),
           GetSystemMetrics(SM_YVIRTUALSCREEN),
           GetSystemMetrics(SM_CXVIRTUALSCREEN),
           GetSystemMetrics(SM_CYVIRTUALSCREEN),
           GetSystemMetrics(SM_REMOTESESSION));

    print_devices(0);
    print_monitors(0);
}

