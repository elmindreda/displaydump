
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

static BOOL CALLBACK monitor_callback(HMONITOR monitor, HDC dc, LPRECT rect, LPARAM data)
{
    MONITORINFOEXW mi;
    char* device;

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);

    if (!GetMonitorInfoW(monitor, (MONITORINFO*) &mi))
        return TRUE;

    device = utf16_to_utf8(mi.szDevice);

    indent(data);
    printf("Monitor Device: \"%s\" Rect: %i,%i,%i,%i", device, rect->left, rect->top, rect->right, rect->bottom);

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
    print_devices(0);
    print_monitors(0);
}

