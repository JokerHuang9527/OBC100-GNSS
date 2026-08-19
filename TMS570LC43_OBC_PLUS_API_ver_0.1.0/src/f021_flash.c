/*
 * f021_flash.c
 *
 *  Created on: 2019¦~11¤ë20¤é
 *      Author: kusoyao
 */

#include "F021_flash.h"
#include "HL_sys_cache.h"


#define TIMEOUT 1000000
//Data Address: 0x00000000 ~ 0x00400000 , ECC Address 0xF0400000 ~ 0xF0480000

uint32_t SectorBaseAddressTable[33] = {
    //BANK0, 16 sectors
    0x00000000, 0x00004000, 0x00008000, 0x0000C000,
    0x00010000, 0x00014000, 0x00018000, 0x00020000,
    0x00040000, 0x00060000, 0x00080000, 0x000C0000,
    0x00100000, 0x00140000, 0x00180000, 0x001C0000,
    //BANK1, 16 Sectoes
    0x00200000, 0x00220000, 0x00240000, 0x00260000,
    0x00280000, 0x002A0000, 0x002C0000, 0x002E0000,
    0x00300000, 0x00320000, 0x00340000, 0x00360000,
    0x00380000, 0x003A0000, 0x003C0000, 0x003E0000,
    //For length calculate
    0x00400000,
};

static Fapi_StatusType F021_WaitReady(int timeout, bool busywait)
{
    Fapi_StatusType status = Fapi_Status_Success;
    while(FAPI_CHECK_FSM_READY_BUSY != Fapi_Status_FsmReady)
    {
        if((--timeout) == 0)
            break;
        if(busywait == false)
            vTaskDelay(1);
    }

    if(timeout == 0)
        status = Fapi_Error_Fail;

    uint32_t fmstat = FAPI_GET_FSM_STATUS;
    //printk("fmstat: %d\n", fmstat);
    if(fmstat != 0)
        status = Fapi_Error_Fail;

    if(status == Fapi_Error_Fail)
        Fapi_issueAsyncCommand(Fapi_ClearStatus);

    return status;
}

Fapi_StatusType F021_Init()
{
    int i;
    Fapi_StatusType status;
    status = Fapi_initializeFlashBanks(110); // uint32_t u32HclkFrequency in MHZ 110MHz
    //status = Fapi_disableBanksForOtpWrite();
    //status = Fapi_disableFsmDoneEvent();

    // Returns the information specific to the compiled version of the API library
    Fapi_LibraryInfoType LibraryInfo = Fapi_getLibraryInfo();
    printk("u8ApiMajorVersion: %d\n", LibraryInfo.u8ApiMajorVersion);
    printk("u8ApiMinorVersion: %d\n", LibraryInfo.u8ApiMinorVersion);
    printk("u8ApiRevision: %d\n", LibraryInfo.u8ApiRevision);
    printk("oApiProductionStatus: %d\n", LibraryInfo.oApiProductionStatus);
    printk("u32ApiBuildNumber: %d\n", LibraryInfo.u32ApiBuildNumber);
    printk("u8ApiTechnologyType: %d\n", LibraryInfo.u8ApiTechnologyType);
    printk("u8ApiTechnologyRevision: %d\n", LibraryInfo.u8ApiTechnologyRevision);
    printk("u8ApiEndianness: %d\n", LibraryInfo.u8ApiEndianness);
    printk("u32ApiCompilerVersion: %d\n", LibraryInfo.u32ApiCompilerVersion);

    //Returns the information specific to the device the API library is being executed on
    Fapi_DeviceInfoType DiviceInfo = Fapi_getDeviceInfo();
    printk("u16NumberOfBanks: %d\n", DiviceInfo.u16NumberOfBanks);
    printk("u16DevicePackage: %d\n", DiviceInfo.u16DevicePackage);
    printk("u16DeviceMemorySize: 0x%x\n", DiviceInfo.u16DeviceMemorySize);
    printk("u32AsicId: %d\n", DiviceInfo.u32AsicId);
    printk("u32LotNumber: %d\n", DiviceInfo.u32LotNumber);
    printk("u16FlowCheck: %d\n", DiviceInfo.u16FlowCheck);
    printk("u16WaferNumber: %d\n", DiviceInfo.u16WaferNumber);
    printk("u16WaferXCoordinate: %d\n", DiviceInfo.u16WaferXCoordinate);
    printk("u16WaferYCoordinate: %d\n", DiviceInfo.u16WaferYCoordinate);

    //Returns the sector information for a bank
    Fapi_FlashBankType oBank = Fapi_FlashBank1;
    Fapi_FlashBankSectorsType poFlashBankSectors;

    status = Fapi_getBankSectors( oBank, &poFlashBankSectors );
    printk("status: %d\n", status);
    if(status == Fapi_Status_Success)
    {
        printk("oFlashBankTech: %d\n", poFlashBankSectors.oFlashBankTech );
        printk("u32NumberOfSectors: %d\n", poFlashBankSectors.u32NumberOfSectors);
        printk("u32BankStartAddress: 0x%x\n", poFlashBankSectors.u32BankStartAddress);
        for(i = 0;i < 16;++i)
            printk("au16SectorSizes[%d]: 0x%x\n", i, poFlashBankSectors.au16SectorSizes[i]);
    }

    uint32_t DataAddress = 0;
    uint32_t ECCAddress = 0;
    for(i = 0; i < 32; ++i)
    {
        DataAddress = SectorBaseAddressTable[i];
        ECCAddress = Fapi_remapMainAddress(DataAddress);
        printk("DataAddress: 0x%x ECCAddress: 0x%x\n", DataAddress, ECCAddress);

        //DataAddress = Fapi_remapEccAddress(ECCAddress);
        //printk("DataAddress: 0x%x\n", DataAddress);
    }

    status = Fapi_enableAutoEccCalculation();

    //The F021 Flash API library cannot be executed from the same bank as the active bank selected for the API commands to operate on.
    Fapi_FlashBankType oNewFlashBank = Fapi_FlashBank1;
    status = Fapi_setActiveFlashBank(oNewFlashBank);
    printk("status: %d\n", status);

    //Bit mask indicating which of sectors 0-15 are enabled for erase and programming.
    uint16_t u16SectorsEnables = 0xFFFF;
    status = Fapi_enableMainBankSectors(u16SectorsEnables);
    printk("status: %d\n", status);

    status = F021_WaitReady(TIMEOUT, true);
    printk("status: %d\n", status);

    Fapi_issueAsyncCommand(Fapi_ClearStatus);

    return status;
}

Fapi_StatusType F021_Erase(uint32_t Sector)
{
    Fapi_StatusType status;
    Fapi_FlashStateCommandsType oCommand = Fapi_EraseSector;

    status = Fapi_issueAsyncCommandWithAddress(oCommand, SectorBaseAddressTable[Sector]);
    if(status != Fapi_Status_Success)
        return status;

    coreInvalidateDCByAddress(SectorBaseAddressTable[Sector], SectorBaseAddressTable[Sector + 1] - SectorBaseAddressTable[Sector]);

    return F021_WaitReady(TIMEOUT, false);
}

Fapi_StatusType F021_Write(uint8_t *dest, uint8_t *src, uint32_t len)
{
    /*
     * Can write to any address
     * Can write any length
     * Write to previous written 4 byte address will fail.
     *   EX: write to address 0x200000 with length 1 will pass,
     *       then write to address 0x200001 with any length will fail,
     *       but write to address from 0x200004 with any length will pass
     *
     *
    */
    Fapi_StatusType status;
    Fapi_FlashProgrammingCommandsType oMode = Fapi_AutoEccGeneration;

    uint32_t ProgramAddress = dest;
    uint32_t DataBufferAddress = src;
    uint32_t l = len;
    uint8_t BytesToProgram;

    while(l > 0)
    {
        if(l > 32)
            BytesToProgram = 32;
        else
            BytesToProgram = l;

        status = Fapi_issueProgrammingCommand( ProgramAddress, DataBufferAddress, BytesToProgram, 0, 0, oMode);
        if(status != Fapi_Status_Success)
            return status;

        status = F021_WaitReady(TIMEOUT, true);
        if(status != Fapi_Status_Success)
            return status;

        DataBufferAddress += BytesToProgram;
        ProgramAddress += BytesToProgram;
        l -= BytesToProgram;
    }

    coreInvalidateDCByAddress(dest, len);

    int d = memcmp(src, dest, len);
    if(d != 0)
        return Fapi_Error_Fail;

    return Fapi_Status_Success;
}


Fapi_StatusType F021_Read(uint8_t *dest, uint8_t *src, uint32_t len)
{
    memcpy(dest, src, len);
    return Fapi_Status_Success;
}
