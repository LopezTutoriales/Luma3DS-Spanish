/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2021 Aurora Wright, TuxSH
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include <3ds.h>
#include "luma_config.h"
#include "menus/sysconfig.h"
#include "memory.h"
#include "draw.h"
#include "fmt.h"
#include "utils.h"
#include "ifile.h"
#include "luminance.h"

Menu sysconfigMenu = {
    "Menu de configuracion del sistema",
    {
        { "Control de volumen", METHOD, .method=&SysConfigMenu_AdjustVolume},
        { "Controlar conexion Wifi", METHOD, .method = &SysConfigMenu_ControlWifi },
        { "Alternar LEDs", METHOD, .method = &SysConfigMenu_ToggleLEDs },
        { "Alternar adaptador Wifi", METHOD, .method = &SysConfigMenu_ToggleWireless },
        { "Alternar boton Power", METHOD, .method=&SysConfigMenu_TogglePowerButton },
        { "Alternar alimentacion del slot de cartucho", METHOD, .method=&SysConfigMenu_ToggleCardIfPower},
        { "Cambiar brillo de las pantallas", METHOD, .method = &SysConfigMenu_ChangeScreenBrightness },
        {},
    }
};

bool isConnectionForced = false;
s8 currVolumeSliderOverride = -1;

void SysConfigMenu_ToggleLEDs(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Menu de configuracion del sistema");
        Draw_DrawString(10, 30, COLOR_WHITE, "Pulsa A para cambiar, Pulsa B para volver.");
        Draw_DrawString(10, 50, COLOR_RED, "ADVERTENCIA:");
        Draw_DrawString(10, 60, COLOR_WHITE, "  * El modo reposo reseteara el estado de los LEDs!");
        Draw_DrawString(10, 70, COLOR_WHITE, "  * No se puede alternar los LEDs con la bateria");
		Draw_DrawString(10, 80, COLOR_WHITE, "  * baja!");

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A)
        {
            mcuHwcInit();
            u8 result;
            MCUHWC_ReadRegister(0x28, &result, 1);
            result = ~result;
            MCUHWC_WriteRegister(0x28, &result, 1);
            mcuHwcExit();
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void SysConfigMenu_ToggleWireless(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    bool nwmRunning = isServiceUsable("nwm::EXT");

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Menu de configuracion del sistema");
        Draw_DrawString(10, 30, COLOR_WHITE, "Pulsa A para cambiar, Pulsa B para volver.");

        u8 wireless = (*(vu8 *)((0x10140000 | (1u << 31)) + 0x180));

        if(nwmRunning)
        {
            Draw_DrawString(10, 50, COLOR_WHITE, "Estado actual:");
            Draw_DrawString(100, 50, (wireless ? COLOR_GREEN : COLOR_RED), (wireless ? " ACTIVADO   " : " DESACTIVADO"));
        }
        else
        {
            Draw_DrawString(10, 50, COLOR_RED, "Administrador de redes no esta iniciado.");
            Draw_DrawString(10, 60, COLOR_RED, "Si estas en modo TEST, sal y luego");
            Draw_DrawString(10, 70, COLOR_RED, "pulsa R+DERECHA para alternar el wifi.");
            Draw_DrawString(10, 80, COLOR_RED, "Si no, solo sal y espera unos segundos.");
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A && nwmRunning)
        {
            nwmExtInit();
            NWMEXT_ControlWirelessEnabled(!wireless);
            nwmExtExit();
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void SysConfigMenu_UpdateStatus(bool control)
{
    MenuItem *item = &sysconfigMenu.items[1];

    if(control)
    {
        item->title = "Controlar conexion Wifi";
        item->method = &SysConfigMenu_ControlWifi;
    }
    else
    {
        item->title = "Desactivar conexion inalambrica forzada";
        item->method = &SysConfigMenu_DisableForcedWifiConnection;
    }
}

static bool SysConfigMenu_ForceWifiConnection(u32 slot)
{
    char ssid[0x20 + 1] = {0};
    isConnectionForced = false;

    if(R_FAILED(acInit()))
        return false;

    acuConfig config = {0};
    ACU_CreateDefaultConfig(&config);
    ACU_SetNetworkArea(&config, 2);
    ACU_SetAllowApType(&config, 1 << slot);
    ACU_SetRequestEulaVersion(&config);

    Handle connectEvent = 0;
    svcCreateEvent(&connectEvent, RESET_ONESHOT);

    bool forcedConnection = false;
    if(R_SUCCEEDED(ACU_ConnectAsync(&config, connectEvent)))
    {
        if(R_SUCCEEDED(svcWaitSynchronization(connectEvent, -1)) && R_SUCCEEDED(ACU_GetSSID(ssid)))
            forcedConnection = true;
    }
    svcCloseHandle(connectEvent);

    if(forcedConnection)
    {
        isConnectionForced = true;
        SysConfigMenu_UpdateStatus(false);
    }
    else
        acExit();

    char infoString[80] = {0};
    u32 infoStringColor = forcedConnection ? COLOR_GREEN : COLOR_RED;
    if(forcedConnection)
        sprintf(infoString, "Conexion forzada a: %s con exito", ssid);
    else
       sprintf(infoString, "Fallo al conectar al slot %d", (int)slot + 1);

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Menu de configuracion del sistema");
        Draw_DrawString(10, 30, infoStringColor, infoString);
        Draw_DrawString(10, 40, COLOR_WHITE, "Pulsa B para volver.");

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_B)
            break;
    }
    while(!menuShouldExit);

    return forcedConnection;
}

void SysConfigMenu_TogglePowerButton(void)
{
    u32 mcuIRQMask;

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    mcuHwcInit();
    MCUHWC_ReadRegister(0x18, (u8*)&mcuIRQMask, 4);
    mcuHwcExit();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Menu de configuracion del sistema");
        Draw_DrawString(10, 30, COLOR_WHITE, "Pulsa A para cambiar, Pulsa B para volver.");

        Draw_DrawString(10, 50, COLOR_WHITE, "Estado actual:");
        Draw_DrawString(100, 50, (((mcuIRQMask & 0x00000001) == 0x00000001) ? COLOR_RED : COLOR_GREEN), (((mcuIRQMask & 0x00000001) == 0x00000001) ? " DESACTIVADO" : " ACTIVADO   "));

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A)
        {
            mcuHwcInit();
            MCUHWC_ReadRegister(0x18, (u8*)&mcuIRQMask, 4);
            mcuIRQMask ^= 0x00000001;
            MCUHWC_WriteRegister(0x18, (u8*)&mcuIRQMask, 4);
            mcuHwcExit();
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void SysConfigMenu_ControlWifi(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    u32 slot = 0;
    char ssids[3][32] = {{0}};

    Result resInit = acInit();
    for (u32 i = 0; i < 3; i++)
    {
        if (R_SUCCEEDED(ACI_LoadNetworkSetting(i)))
            ACI_GetNetworkWirelessEssidSecuritySsid(ssids[i]);
        else
            strcpy(ssids[i], "(no configurado)");
    }
    if (R_SUCCEEDED(resInit))
        acExit();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Menu de configuracion del sistema");
        u32 posY = Draw_DrawString(10, 30, COLOR_WHITE, "Pulsa A para fozar conexion al slot, B para volver\n\n");

        for (u32 i = 0; i < 3; i++)
        {
            Draw_DrawString(10, posY + SPACING_Y * i, COLOR_TITLE, slot == i ? ">" : " ");
            Draw_DrawFormattedString(30, posY + SPACING_Y * i, COLOR_WHITE, "[%d] %s", (int)i + 1, ssids[i]);
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A)
        {
            if(SysConfigMenu_ForceWifiConnection(slot))
            {
                // Connection successfully forced, return from this menu to prevent ac handle refcount leakage.
                break;
            }

            Draw_Lock();
            Draw_ClearFramebuffer();
            Draw_FlushFramebuffer();
            Draw_Unlock();
        }
        else if(pressed & KEY_DOWN)
        {
            slot = (slot + 1) % 3;
        }
        else if(pressed & KEY_UP)
        {
            slot = (3 + slot - 1) % 3;
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void SysConfigMenu_DisableForcedWifiConnection(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    acExit();
    SysConfigMenu_UpdateStatus(true);

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Menu de configuracion del sistema");
        Draw_DrawString(10, 30, COLOR_WHITE, "Conexion forzada desactivada correctamente.\nNota: La conexion automatica puede estropearse.");

        u32 pressed = waitInputWithTimeout(1000);
        if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void SysConfigMenu_ToggleCardIfPower(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    bool cardIfStatus = false;
    bool updatedCardIfStatus = false;

    do
    {
        Result res = FSUSER_CardSlotGetCardIFPowerStatus(&cardIfStatus);
        if (R_FAILED(res)) cardIfStatus = false;

        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Menu de configuracion del sistema");
        u32 posY = Draw_DrawString(10, 30, COLOR_WHITE, "Pulsa A para cambiar, pulsa B para volver.\n\n");
        posY = Draw_DrawString(10, posY, COLOR_WHITE, "Insertar o quitar un cartucho reseteara el estado,\nY necesitaras reinsertar el cartucho si quieres\njugar con el.\n\n");
        Draw_DrawString(10, posY, COLOR_WHITE, "Estado actual:");
        Draw_DrawString(100, posY, !cardIfStatus ? COLOR_RED : COLOR_GREEN, !cardIfStatus ? " DESACTIVADO" : " ACTIVADO   ");

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A)
        {
            if (!cardIfStatus)
                res = FSUSER_CardSlotPowerOn(&updatedCardIfStatus);
            else
                res = FSUSER_CardSlotPowerOff(&updatedCardIfStatus);

            if (R_SUCCEEDED(res))
                cardIfStatus = !updatedCardIfStatus;
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

static Result SysConfigMenu_ApplyVolumeOverride(void)
{
    // This feature repurposes the functionality used for the camera shutter sound.
    // As such, it interferes with it:
    //     - shutter volume is set to the override instead of its default 100% value
    //     - due to implementation details, having the shutter sound effect play will
    //       make this feature stop working until the volume override is reapplied by
    //       going back to this menu

    // Original credit: profi200

    u8 i2s1Mux;
    u8 i2s2Mux;
    Result res = cdcChkInit();

    if (R_SUCCEEDED(res)) res = CDCCHK_ReadRegisters2(0,  116, &i2s1Mux, 1); // used for shutter sound in TWL mode, and all GBA/DSi/3DS application
    if (R_SUCCEEDED(res)) res = CDCCHK_ReadRegisters2(100, 49, &i2s2Mux, 1); // used for shutter sound in CTR mode and CTR mode library applets

    if (currVolumeSliderOverride >= 0)
    {
        i2s1Mux &= ~0x80;
        i2s2Mux |=  0x20;
    }
    else
    {
        i2s1Mux |=  0x80;
        i2s2Mux &= ~0x20;
    }

    s8 i2s1Volume;
    s8 i2s2Volume;
    if (currVolumeSliderOverride >= 0)
    {
        i2s1Volume = -128 + (((float)currVolumeSliderOverride/100.f) * 108);
        i2s2Volume = i2s1Volume;
    }
    else
    {
        // Restore shutter sound volumes. This sould be sourced from cfg,
        // however the values are the same everwhere
        i2s1Volume =  -3; // -1.5 dB (115.7%, only used by TWL applications when taking photos)
        i2s2Volume = -20; // -10 dB  (100%)
    }

    // Write volume overrides values before writing to the pinmux registers
    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(0, 65, &i2s1Volume, 1); // CDC_REG_DAC_L_VOLUME_CTRL
    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(0, 66, &i2s1Volume, 1); // CDC_REG_DAC_R_VOLUME_CTRL
    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(100, 123, &i2s2Volume, 1);

    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(0, 116, &i2s1Mux, 1);
    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(100, 49, &i2s2Mux, 1);

    cdcChkExit();
    return res;
}

void SysConfigMenu_LoadConfig(void)
{
    s64 out = -1;
    svcGetSystemInfo(&out, 0x10000, 7);
    currVolumeSliderOverride = (s8)out;
    if (currVolumeSliderOverride >= 0)
        SysConfigMenu_ApplyVolumeOverride();
}

void SysConfigMenu_AdjustVolume(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    s8 tempVolumeOverride = currVolumeSliderOverride;
    static s8 backupVolumeOverride = -1;
    if (backupVolumeOverride == -1)
        backupVolumeOverride = tempVolumeOverride >= 0 ? tempVolumeOverride : 85;

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Menu de configuracion del sistema");
        u32 posY = Draw_DrawString(10, 30, COLOR_WHITE, "Y: Anular/Activar control de volumen fisico.\nCRUCETA/CPAD: Ajustar nivel del volumen.\nA: Aplicar\nB: Volver\n\n");
        Draw_DrawString(10, posY, COLOR_WHITE, "Estado actual:");
        posY = Draw_DrawString(100, posY, (tempVolumeOverride == -1) ? COLOR_RED : COLOR_GREEN, (tempVolumeOverride == -1) ? " DESACTIVADO" : " ACTIVADO   ");
        if (tempVolumeOverride != -1) {
            posY = Draw_DrawFormattedString(30, posY, COLOR_WHITE, "\nValor: [%d%%]    ", tempVolumeOverride);
        } else {
            posY = Draw_DrawString(30, posY, COLOR_WHITE, "\n                 ");
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        Draw_Lock();

        if(pressed & KEY_A)
        {
            currVolumeSliderOverride = tempVolumeOverride;
            Result res = SysConfigMenu_ApplyVolumeOverride();
            LumaConfig_SaveSettings();
            if (R_SUCCEEDED(res))
                Draw_DrawString(10, posY, COLOR_GREEN, "\nExito!");
            else
                Draw_DrawFormattedString(10, posY, COLOR_RED, "\nFallo: 0x%08lX", res);
        }
        else if(pressed & KEY_B)
            return;
        else if(pressed & KEY_Y)
        {
            Draw_DrawString(10, posY, COLOR_WHITE, "\n                 ");
            if (tempVolumeOverride == -1) {
                tempVolumeOverride = backupVolumeOverride;
            } else {
                backupVolumeOverride = tempVolumeOverride;
                tempVolumeOverride = -1;
            }
        }
        else if ((pressed & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) && tempVolumeOverride != -1)
        {
            Draw_DrawString(10, posY, COLOR_WHITE, "\n                 ");
            if (pressed & KEY_UP)
                tempVolumeOverride++;
            else if (pressed & KEY_DOWN)
                tempVolumeOverride--;
            else if (pressed & KEY_RIGHT)
                tempVolumeOverride+=10;
            else if (pressed & KEY_LEFT)
                tempVolumeOverride-=10;

            if (tempVolumeOverride < 0)
                tempVolumeOverride = 0;
            if (tempVolumeOverride > 100)
                tempVolumeOverride = 100;
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();
    } while(!menuShouldExit);
}

void SysConfigMenu_ChangeScreenBrightness(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    // gsp:LCD GetLuminance is stubbed on O3DS so we have to implement it ourselves... damn it.
    // Assume top and bottom screen luminances are the same (should be; if not, we'll set them to the same values).
    u32 luminance = getCurrentLuminance(false);
    u32 minLum = getMinLuminancePreset();
    u32 maxLum = getMaxLuminancePreset();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Brillo de las pantallas");
        u32 posY = 30;
        posY = Draw_DrawFormattedString(
            10,
            posY,
            COLOR_WHITE,
            "Brillo actual: %lu (min. %lu, max. %lu)\n\n",
            luminance,
            minLum,
            maxLum
        );
        posY = Draw_DrawString(10, posY, COLOR_WHITE, "Controles: Arr/Aba para +-1, Der/Izq para +-10.\n");
        posY = Draw_DrawString(10, posY, COLOR_WHITE, "Pulsa A para empezar, B para salir.\n\n");

        posY = Draw_DrawString(10, posY, COLOR_RED, "ADVERTENCIA: \n");
        posY = Draw_DrawString(10, posY, COLOR_WHITE, "  * El valor estara limitado por la\ncalibracion.\n");
        posY = Draw_DrawString(10, posY, COLOR_WHITE, "  * El framebuffer inferior se restaurara\ncuando salgas.");
        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if (pressed & KEY_A)
            break;

        if (pressed & KEY_B)
            return;
    }
    while (!menuShouldExit);

    Draw_Lock();

    Draw_RestoreFramebuffer();
    Draw_FreeFramebufferCache();

    svcKernelSetState(0x10000, 2); // unblock gsp
    gspLcdInit(); // assume it doesn't fail. If it does, brightness won't change, anyway.

    // gsp:LCD will normalize the brightness between top/bottom screen, handle PWM, etc.

    s32 lum = (s32)luminance;

    do
    {
        u32 pressed = waitInputWithTimeout(1000);
        if (pressed & DIRECTIONAL_KEYS)
        {
            if (pressed & KEY_UP)
                lum += 1;
            else if (pressed & KEY_DOWN)
                lum -= 1;
            else if (pressed & KEY_RIGHT)
                lum += 10;
            else if (pressed & KEY_LEFT)
                lum -= 10;

            lum = lum < (s32)minLum ? (s32)minLum : lum;
            lum = lum > (s32)maxLum ? (s32)maxLum : lum;

            // We need to call gsp here because updating the active duty LUT is a bit tedious (plus, GSP has internal state).
            // This is actually SetLuminance:
            GSPLCD_SetBrightnessRaw(BIT(GSP_SCREEN_TOP) | BIT(GSP_SCREEN_BOTTOM), lum);
        }

        if (pressed & KEY_B)
            break;
    }
    while (!menuShouldExit);

    gspLcdExit();
    svcKernelSetState(0x10000, 2); // block gsp again

    if (R_FAILED(Draw_AllocateFramebufferCache(FB_BOTTOM_SIZE)))
    {
        // Shouldn't happen
        __builtin_trap();
    }
    else
        Draw_SetupFramebuffer();

    Draw_Unlock();
}
