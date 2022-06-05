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
#include "menus/sysconfig.h"
#include "memory.h"
#include "draw.h"
#include "fmt.h"
#include "utils.h"
#include "ifile.h"

Menu sysconfigMenu = {
    "Menu de configuracion del sistema",
    {
        { "Controlar conexion Wifi", METHOD, .method = &SysConfigMenu_ControlWifi },
        { "Alternar LEDs", METHOD, .method = &SysConfigMenu_ToggleLEDs },
        { "Alternar adaptador Wifi", METHOD, .method = &SysConfigMenu_ToggleWireless },
        { "Alternar boton Power", METHOD, .method=&SysConfigMenu_TogglePowerButton },
        { "Alternar alimentacion del slot de cartucho", METHOD, .method=&SysConfigMenu_ToggleCardIfPower},
        {},
    }
};

bool isConnectionForced = false;

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
            Draw_DrawString(100, 50, (wireless ? COLOR_GREEN : COLOR_RED), (wireless ? " ACTIVADO " : " DESACTIVADO"));
        }
        else
        {
            Draw_DrawString(10, 50, COLOR_RED, "Administrador de redes no esta ejecutandose.");
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
    MenuItem *item = &sysconfigMenu.items[0];

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

static bool SysConfigMenu_ForceWifiConnection(int slot)
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
       sprintf(infoString, "Fallo al conectar al slot %d", slot + 1);

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
        Draw_DrawString(100, 50, (((mcuIRQMask & 0x00000001) == 0x00000001) ? COLOR_RED : COLOR_GREEN), (((mcuIRQMask & 0x00000001) == 0x00000001) ? " DESACTIVADO" : " ACTIVADO "));

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

    int slot = 0;
    char slotString[12] = {0};
    sprintf(slotString, ">1<  2   3 ");
    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, "Menu de configuracion del sistema");
        Draw_DrawString(10, 30, COLOR_WHITE, "Pulsa A para fozar una conexion al slot:");
        Draw_DrawString(10, 40, COLOR_WHITE, slotString);
        Draw_DrawString(10, 60, COLOR_WHITE, "Pulsa B para volver.");

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
        else if(pressed & KEY_LEFT)
        {
            slotString[slot * 4] = ' ';
            slotString[(slot * 4) + 2] = ' ';
            slot--;
            if(slot == -1)
                slot = 2;
            slotString[slot * 4] = '>';
            slotString[(slot * 4) + 2] = '<';
        }
        else if(pressed & KEY_RIGHT)
        {
            slotString[slot * 4] = ' ';
            slotString[(slot * 4) + 2] = ' ';
            slot++;
            if(slot == 3)
                slot = 0;
            slotString[slot * 4] = '>';
            slotString[(slot * 4) + 2] = '<';
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
        Draw_DrawString(100, posY, !cardIfStatus ? COLOR_RED : COLOR_GREEN, !cardIfStatus ? " DESACTIVADO" : " ACTIVADO ");

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
