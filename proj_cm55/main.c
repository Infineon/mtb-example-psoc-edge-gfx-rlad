/*****************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for CM55 CPU
*
* Related Document : See README.md
*
******************************************************************************
* (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*****************************************************************************/
#include "retarget_io_init.h"

#include "mtb_disp_dsi_waveshare_4p3.h"

#include "cy_graphics.h"
#include "infineon_logo_800_480.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define DISP_DELAY_MS      (500u)
#define LOOP_DELAY_MS      (100u)
#define DISP_RLAD_FORMAT   (4u)
#define DISP_LAYER_STATE   (1u)
#define WORD_SIZE_BYTES    (4u)

#define I2C_CONTROLLER_IRQ_PRIORITY         (2UL)

/*******************************************************************************
* Global Variables
*******************************************************************************/
GFXSS_Type* base = (GFXSS_Type*) GFXSS;

CY_SECTION(".cy_gpu_buf") uint8_t gfx_buff[SIZE_IN_WORDS * WORD_SIZE_BYTES] = { 0xFF };

/* RLAD configurations */
cy_stc_gfx_rlad_cfg_t rlad_cfg =
{
    .layer_id = GFX_LAYER_GRAPHICS,                    /* Overlay0 Layer ID */
    .image_width = (IMG_PTR_WIDTH - 1U),               /* Image width */
    .image_height = (IMG_PTR_HEIGHT - 1U),             /* Image height */
    .compressed_image_size = (SIZE_IN_WORDS - 1UL),    /* compressed image size */
    .image_address = (uint32_t*)gfx_buff,               /* Image address */
    .compression_mode = CY_GFX_RLAD_MODE_RLAD,         /* RLAD compression mode */
    .rlad_format = CY_GFX_RLAD_FMT_RGB888,                   /* RLAD display format */
    .enable = DISP_LAYER_STATE,                        /* Layer state */
};

cy_stc_scb_i2c_context_t disp_i2c_controller_context;

cy_stc_sysint_t disp_i2c_controller_irq_cfg =
{
    .intrSrc      = CYBSP_I2C_CONTROLLER_IRQ,
    .intrPriority = I2C_CONTROLLER_IRQ_PRIORITY,
};


/*******************************************************************************
* Function Name: disp_i2c_controller_interrupt
********************************************************************************
* Summary:
*  I2C controller ISR which invokes Cy_SCB_I2C_Interrupt to perform I2C transfer
*  as controller.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void disp_i2c_controller_interrupt(void)
{
    Cy_SCB_I2C_Interrupt(CYBSP_I2C_CONTROLLER_HW, &disp_i2c_controller_context);
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  This is the main function for CM55 non-secure application.
*    1. It initializes the device and board peripherals.
*    2. Enables RLAD decoder, decompresses the RLAD encoded image and renders it
*       on 4.3-inch LCD display.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    cy_en_scb_i2c_status_t i2c_result = CY_SCB_I2C_SUCCESS;
    cy_rslt_t panel_status = CY_RSLT_SUCCESS;
    volatile cy_en_gfx_status_t status;
    cy_en_sysint_status_t sysint_status = CY_SYSINT_SUCCESS;
    cy_stc_gfx_context_t gfx_context;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Enable global interrupts */
    __enable_irq();

    init_retarget_io();

    memcpy(gfx_buff, img_ptr, sizeof(img_ptr));

    /* Setting graphics frame buffers address to gfx_buff address */
    GFXSS_config.dc_cfg->gfx_layer_config->buffer_address = (size_t* )&gfx_buff;
    GFXSS_config.dc_cfg->gfx_layer_config->uv_buffer_address = (size_t* )&gfx_buff;

    /* Initializes the graphics system according to the configuration */
    status = Cy_GFXSS_Init(base, &GFXSS_config, &gfx_context);
    if(CY_GFX_SUCCESS == status)
    {
        /* Initialize the I2C in controller mode. */
        i2c_result = Cy_SCB_I2C_Init(CYBSP_I2C_CONTROLLER_HW,
                    &CYBSP_I2C_CONTROLLER_config, &disp_i2c_controller_context);

        if (CY_SCB_I2C_SUCCESS != i2c_result)
        {
            printf("I2C controller initialization failed !!\n");
            handle_app_error();
        }

        /* Initialize the I2C interrupt */
        sysint_status = Cy_SysInt_Init(&disp_i2c_controller_irq_cfg,
                                        &disp_i2c_controller_interrupt);

        if (CY_SYSINT_SUCCESS != sysint_status)
        {
            printf("I2C controller interrupt initialization failed\r\n");
            handle_app_error();
        }

        /* Enable the I2C interrupts. */
        NVIC_EnableIRQ(disp_i2c_controller_irq_cfg.intrSrc);

        /* Enable the I2C */
        Cy_SCB_I2C_Enable(CYBSP_I2C_CONTROLLER_HW);

        /* Initialize the display */
        panel_status = mtb_disp_waveshare_4p3_init(CYBSP_I2C_CONTROLLER_HW,
                                                    &disp_i2c_controller_context);

        if (CY_RSLT_SUCCESS != panel_status)
        {
            printf("Waveshare 4.3-Inch R-Pi display init failed with status = %u\r\n", (unsigned int) panel_status);
            handle_app_error();
        }

        /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
        printf("\x1b[2J\x1b[;H");

        printf("****************** "
                "PSOC Edge MCU: Graphics using RLAD "
                "****************** \r\n\n");

        printf("Graphics subsystem initialization successful \r\n");

        /* Set RLAD config and enable RLAD */
        Cy_GFXSS_RLAD_SetImage(base, &rlad_cfg, &gfx_context);
        Cy_GFXSS_RLAD_Enable(base, &gfx_context);

        /* Set frame address as base address of compressed image */
        Cy_GFXSS_Set_FrameBuffer(base, rlad_cfg.image_address, &gfx_context);

        Cy_SysLib_Delay(DISP_DELAY_MS);

        switch(Cy_GFXSS_Get_RLAD_Status(base))
        {
            case CY_GFX_RLAD_NORMAL_OPERATION:
                printf("RLAD decoded with normal operation.\n\r");
                break;
            case CY_GFX_RLAD_AXI_ERROR:
                printf("An AXI error response was received when reading compressed image data.\n\r");
                break;
            case CY_GFX_RLAD_BUF_TOO_SMALL:
                printf("The RLAD_SIZE setting is inconsistent with the compressed image data (image decompression not complete when end of buffer was reached).\n\r");
                break;
            case CY_GFX_RLAD_BUF_TOO_LARGE:
                printf("The RLAD_SIZE setting is inconsistent with the compressed image data (image decompression completed before end of buffer was reached).\n\r");
                break;
            case CY_GFX_RLAD_DC_INVALID_SIZE:
                printf("The Display Controller was reading the first pixel of a frame when it was not expected. This means an inconsistent setup of RLAD and DC configuration.\n\r");
                break;
            case CY_GFX_RLAD_DC_INVALID_FORMAT:
                printf("The Display Controller was reading data with an unexpected burst length. This means an invalid buffer format was configured.\n\r");
                break;
        }
        Cy_GFXSS_Interrupt(base, &gfx_context);
    }

    for (;;)
    {
        /* Enters CPU Sleep */
        Cy_SysPm_CpuEnterSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
    }
}

/* [] END OF FILE */