/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2020 Aurora Wright, TuxSH
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
#include <math.h>
#include "fmt.h"
#include "menus/n3ds.h"
#include "memory.h"
#include "menu.h"
#include "n3ds.h"
#include "draw.h"

static char clkRateBuf[128 + 1];

static QtmCalibrationData lastQtmCal = {0};
static bool qtmCalRead = false;

Menu N3DSMenu = {
    "Menu para New 3DS",
    {
        { "Habilitar cache L2", METHOD, .method = &N3DSMenu_EnableDisableL2Cache },
        { clkRateBuf, METHOD, .method = &N3DSMenu_ChangeClockRate },
        { "Dessactivar temporalmente 3D Super-Estable", METHOD, .method = &N3DSMenu_ToggleSs3d, .visibility = &N3DSMenu_CheckNotN2dsXl },
        { "Testear posiciones de barra de paralaje", METHOD, .method = &N3DSMenu_TestBarrierPositions, .visibility = &N3DSMenu_CheckNotN2dsXl },
        { "Calibracion de 3D Super-Estable", METHOD, .method = &N3DSMenu_Ss3dCalibration, .visibility = &N3DSMenu_CheckNotN2dsXl },
        {},
    }
};

static s64 clkRate = 0, higherClkRate = 0, L2CacheEnabled = 0;
static bool qtmUnavailableAndNotBlacklisted = false; // true on N2DSXL, though we check MCU system model data first
static QtmStatus lastUpdatedQtmStatus;

bool N3DSMenu_CheckNotN2dsXl(void)
{
    // Also check if qtm could be initialized
    return isQtmInitialized && mcuInfoTableRead && mcuInfoTable[9] < 5 && !qtmUnavailableAndNotBlacklisted;
}

void N3DSMenu_UpdateStatus(void)
{
    svcGetSystemInfo(&clkRate, 0x10001, 0);
    svcGetSystemInfo(&higherClkRate, 0x10001, 1);
    svcGetSystemInfo(&L2CacheEnabled, 0x10001, 2);

    N3DSMenu.items[0].title = L2CacheEnabled ? "Deshabilitar cache L2" : "Habilitar cache L2";
    sprintf(clkRateBuf, "Establecer frecuenc. del reloj a %luMHz", clkRate != 268 ? 268 : (u32)higherClkRate);

    if (N3DSMenu_CheckNotN2dsXl())
    {
        bool blacklisted = false;

        // Read status
        if (R_FAILED(QTMS_GetQtmStatus(&lastUpdatedQtmStatus)))
            qtmUnavailableAndNotBlacklisted = true; // stop showing QTM options if unavailable but not blacklisted

        else if (lastUpdatedQtmStatus == QTM_STATUS_UNAVAILABLE)
            qtmUnavailableAndNotBlacklisted = R_FAILED(QTMU_IsCurrentAppBlacklisted(&blacklisted)) || !blacklisted;


        MenuItem *item = &N3DSMenu.items[2];

        if (lastUpdatedQtmStatus == QTM_STATUS_ENABLED)
            item->title = "Dessactivar temporalmente 3D Super-Estable";
        else
            item->title = "Activar temporalmente 3D Super-Estable";
    }
}

void N3DSMenu_ChangeClockRate(void)
{
    N3DSMenu_UpdateStatus();

    s64 newBitMask = (L2CacheEnabled << 1) | ((clkRate != 268 ? 1 : 0) ^ 1);
    svcKernelSetState(10, (u32)newBitMask);

    N3DSMenu_UpdateStatus();
}

void N3DSMenu_EnableDisableL2Cache(void)
{
    N3DSMenu_UpdateStatus();

    s64 newBitMask = ((L2CacheEnabled ^ 1) << 1) | (clkRate != 268 ? 1 : 0);
    svcKernelSetState(10, (u32)newBitMask);

    N3DSMenu_UpdateStatus();
}

void N3DSMenu_ToggleSs3d(void)
{
    if (qtmUnavailableAndNotBlacklisted)
        return;

    if (lastUpdatedQtmStatus == QTM_STATUS_ENABLED)
        QTMS_SetQtmStatus(QTM_STATUS_SS3D_DISABLED);
    else // both SS3D disabled and unavailable/blacklisted states
        QTMS_SetQtmStatus(QTM_STATUS_ENABLED);

    N3DSMenu_UpdateStatus();
}

void N3DSMenu_TestBarrierPositions(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    u8 pos = 0;
    QTMS_DisableAutoBarrierControl(); // assume it doesn't fail
    QTMS_GetCurrentBarrierPosition(&pos);

    u32 pressed = 0;

    do
    {
        Draw_Lock();

        Draw_DrawString(10, 10, COLOR_TITLE, "Menu para New 3DS");
        u32 posY = Draw_DrawString(10, 30, COLOR_WHITE, "Usa izquierda/derecha para ajustar la posicion de\nla barrera.\n\n");
        posY = Draw_DrawString(10, posY, COLOR_WHITE, "Cada posicion corresponde a un movimiento ocular\nhorizontal de 5,2mm (suponiendo condiciones de\nvisualizacion ideales).\n\n");
        posY = Draw_DrawString(10, posY, COLOR_WHITE, "Una vez que determine la posicion central ideal,\npodra usarla en el submenu de calibracion.\n\n");
        posY = Draw_DrawString(10, posY, COLOR_WHITE, "El comportamiento de ajuste de barrera automatica\nse restablece al salir.\n\n");

        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Posicion de la barrera: %2hhu\n", pos);

        Draw_FlushFramebuffer();
        pressed = waitInputWithTimeout(1000);

        if (pressed & KEY_LEFT)
        {
            pos = pos > 11 ? 11 : pos; // pos is 13 when SS3D is disabled/camera is in use
            pos = (12 + pos - 1) % 12;
            QTMS_SetBarrierPosition(pos);
        }
        else if (pressed & KEY_RIGHT)
        {
            pos = pos > 11 ? 11 : pos; // pos is 13 when SS3D is disabled/camera is in use
            pos = (pos + 1) % 12;
            QTMS_SetBarrierPosition(pos);
        }

        Draw_Unlock();
    } while(!menuShouldExit && !(pressed & KEY_B));

    QTMS_EnableAutoBarrierControl();
}

void N3DSMenu_Ss3dCalibration(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    u8 currentPos;
    QtmTrackingData trackingData = {0};

    if (R_FAILED(QTMS_GetCurrentBarrierPosition(&currentPos)))
        currentPos = 13;

    bool calReadFailed = false;

    if (!qtmCalRead)
    {
        cfguInit();
        calReadFailed =  R_FAILED(CFG_GetConfigInfoBlk8(sizeof(QtmCalibrationData), QTM_CAL_CFG_BLK_ID, &lastQtmCal));
        cfguExit();
        if (!calReadFailed)
            qtmCalRead = true;
    }

    bool trackingDisabled = currentPos == 13;

    if (currentPos < 12)
        QTMS_EnableAutoBarrierControl(); // assume this doesn't fail

    u32 pressed = 0;
    do
    {
        if (!trackingDisabled)
        {
            QTMS_GetCurrentBarrierPosition(&currentPos);
            QTMU_GetTrackingData(&trackingData);
        }

        Draw_Lock();

        Draw_DrawString(10, 10, COLOR_TITLE, "Menu para New 3DS");
        u32 posY = 30;

        if (trackingDisabled)
            posY = Draw_DrawString(10, posY, COLOR_WHITE, "3D Super-Estable desactivado o camara en uso.\nPulsa B para salir de este menu.\n");
        else if (calReadFailed)
            posY = Draw_DrawString(10, posY, COLOR_WHITE, "Fallo al leer datos de calibracion.\nPulsa B para salir de este menu.\n");
        else
        {
            posY = Draw_DrawString(10, posY, COLOR_WHITE, "Der/Izq:     +- 1\n");
            posY = Draw_DrawString(10, posY, COLOR_WHITE, "Arr/Aba:     +- 0.1\n");
            posY = Draw_DrawString(10, posY, COLOR_WHITE, "R/L:         +- 0.01\n");
            posY = Draw_DrawString(10, posY, COLOR_WHITE, "A:           Guardar en configuracion del sistema\n");
            posY = Draw_DrawString(10, posY, COLOR_WHITE, "Y:           Cargar ultima calibracion guardada\n");
            posY = Draw_DrawString(10, posY, COLOR_WHITE, "B:           Salir de este menu\n\n");

            char calStr[16];
            floatToString(calStr, lastQtmCal.centerBarrierPosition, 2, true);
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Posicion central:              %-5s\n\n", calStr);

            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Posicion actual de la barrera: %-2hhu\n", currentPos);
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Distancia ocular actual:       %-2d cm\n", (int)roundf(qtmEstimateEyeToCameraDistance(&trackingData) / 10.0f));
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Distancia ocular optima:       %-2d cm\n", (int)roundf(lastQtmCal.viewingDistance / 10.0f));
        }

        Draw_FlushFramebuffer();
        pressed = waitInputWithTimeout(15);

        if (!calReadFailed)
        {
            if (pressed & KEY_LEFT)
            {
                lastQtmCal.centerBarrierPosition = fmodf(12.0f + lastQtmCal.centerBarrierPosition - 1.0f, 12.0f);
                QTMS_SetCalibrationData(&lastQtmCal, false);
            }
            else if (pressed & KEY_RIGHT)
            {
                lastQtmCal.centerBarrierPosition = fmodf(lastQtmCal.centerBarrierPosition + 1.0f, 12.0f);
                QTMS_SetCalibrationData(&lastQtmCal, false);
            }

            if (pressed & KEY_DOWN)
            {
                lastQtmCal.centerBarrierPosition = fmodf(12.0f + lastQtmCal.centerBarrierPosition - 0.1f, 12.0f);
                QTMS_SetCalibrationData(&lastQtmCal, false);
            }
            else if (pressed & KEY_UP)
            {
                lastQtmCal.centerBarrierPosition = fmodf(lastQtmCal.centerBarrierPosition + 0.1f, 12.0f);
                QTMS_SetCalibrationData(&lastQtmCal, false);
            }

            if (pressed & KEY_L)
            {
                lastQtmCal.centerBarrierPosition = fmodf(12.0f + lastQtmCal.centerBarrierPosition - 0.01f, 12.0f);
                QTMS_SetCalibrationData(&lastQtmCal, false);
            }
            else if (pressed & KEY_R)
            {
                lastQtmCal.centerBarrierPosition = fmodf(lastQtmCal.centerBarrierPosition + 0.01f, 12.0f);
                QTMS_SetCalibrationData(&lastQtmCal, false);
            }

            if (pressed & KEY_A)
                QTMS_SetCalibrationData(&lastQtmCal, true);

            if (pressed & KEY_Y)
            {
                cfguInit();
                calReadFailed =  R_FAILED(CFG_GetConfigInfoBlk8(sizeof(QtmCalibrationData), QTM_CAL_CFG_BLK_ID, &lastQtmCal));
                cfguExit();
                qtmCalRead = !calReadFailed;
                if (qtmCalRead)
                    QTMS_SetCalibrationData(&lastQtmCal, false);
            }
        }

        Draw_Unlock();
    } while(!menuShouldExit && !(pressed & KEY_B));
}
