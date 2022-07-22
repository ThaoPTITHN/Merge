/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "FATFS.h"
#include "HAL.h"
/*Define Macro các ham read_second , Read_minute, ...*/
#define READ_SECOND(a) (2*(a & ((0 | 31))))
#define READ_MINUTE(a) (( a>> 5) & (0 | 63))
#define READ_HOUR(a)   ((a >> 11)&( 0 | 63))
#define READ_DAY(a)    (a & (0 | 31))
#define READ_MONTH(a)  ((a >> 5)&(0 | 15))
#define READ_YEAR(a)   ((a & 0xFE00) >> 9)
/*Define offset Entry*/
#define OFFSET_FILE_NAME    		       0x00
#define OFFSET_NAME_EXTEND  			   0x08
#define OFFSET_PROPER_STATE 	     	   0x0B
#define OFFSET_EXCLUSESIVE  		       0x0C
#define OFFSET_CREATE_TIME       		   0x0D
#define OFFSET_CREATE_DATE        		   0x10
#define OFFSET_DATE_ACCESS_RECENTLY		   0x12
#define OFFSET_CLUSTER_BEGIN_BYTE_H        0x14
#define OFFSET_REVISE_LAST_TIME     	   0x16
#define OFFSET_DATE_UPDATE_LAST    	       0x18
#define OFFSET_CLUSTER_START_BYTE_L        0x1A
#define OFFSET_SIZE_FILE                   0x1C  
/*Offset Boot sector of FAT12*/
#define BYTE_PER_SECTOR              0x0B     /*Number of bytes of sectors*/
#define SECTOR_PER_CLUSTER           0x0D     /*Number of Sector of Cluster*/
#define NUMBER_OF_SECTOR_BEFORE_FAT  0x0E     /*Number of sectors before the FAT table*/
#define NUMBER_OF_FAT                0x10     /*Number of FAT12*/
#define NUMBER_ENTRY_OF_RDET         0x11     /*Number Entry of RDET*/
#define TOTAL_SECTOR_COUNT           0x13     /*Number of sector of volume*/
#define NUMBER_SECTOR_OF_FAT         0x16     /*Number sector of fat */ 
#define TOTAL_SECTOR_COUNT_FAT32     0x20     /*Size of volume*/

#define GET2BYTE(buff) (((uint8_t*)(buff))[0] | (((uint8_t*)(buff))[1] << 8))
#define GET4BYTE(buff) (((uint8_t*)(buff))[0] | (((uint8_t*)(buff))[1] << 8) | (((uint8_t*)(buff))[2] << 16)\
                        | (((uint8_t*)(buff))[3] << 24))
static uint32_t s_endOfFile = 0;
static uint32_t s_sectorPerSize = 0;
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
 /**
* @brief Check FAT type.
*
* @param[in] nameInput: string name
* @returns FATFS_FAT_Version_t: FAT12, FAT16 or FAT_undefine
*/
static FATFS_FAT_Version_t Check_versionFAT(uint8_t *nameInput);
/**
* @brief Read a cluster.
*
* @param[in] bootSector: infor of bootsector area
* @param[in] indexCluster
* @param[inout] None
* @returns none
*/
static void FATFS_ReadCluster(FATFS_BootInfor_Struct_t *bootSector, uint32_t indexCluster);
/**
* @brief Calculate cluster next of FAT12
*
* @param[in] firstCluster: start of cluster
* @returns cluster next
*/
static uint16_t FATFS_Cluster_Next_12(FATFS_BootInfor_Struct_t *bootInfor, uint32_t firstCluster);
/**
* @brief Get cluster next.
*
* @param[in] firstCluster: start of cluster
* @returns cluster next
*/
static uint32_t FATFS_Get_NextCluster(FATFS_BootInfor_Struct_t *bootInfor, uint32_t firstCluster);
/**
* @brief Read infor entry.
*
* @param[in] inforEntry: inforEntry buffFATay with size 32
* @param[out] entry: infor of entry (date, time, size, startCluster,..)
* @returns none
*/
static void FATFS_Read_RootEntry(uint8_t inforEntry[32], FATFS_Entry_Struct_t *entry);

/*******************************************************************************
 * Codes
 ******************************************************************************/
static FATFS_FAT_Version_t Check_versionFAT(uint8_t *nameInput)
{
    FATFS_FAT_Version_t versionFAT = FAT_undefine_version;

    if(strcmp(nameInput, "FAT12") == 0)
    {
        versionFAT = FAT12_version;
    }
    else if(strcmp(nameInput, "FAT16") == 0)
    {
        versionFAT = FAT16_version;
    }
    else
    {
        versionFAT = FAT_undefine_version;
    }

    return versionFAT;
}

static uint32_t FATFS_Get_NextCluster(FATFS_BootInfor_Struct_t *bootInfor, uint32_t firstCluster)
{
    uint32_t retVal = 0;
    uint32_t indexFAT = 0;
    uint8_t buff[32] = {0};
    
    if(s_endOfFile == 0xFFF)
    {
        retVal = FATFS_Cluster_Next_12(bootInfor,firstCluster);
    }
    else if(s_endOfFile == 0xFFFF)
    {
        indexFAT = firstCluster * 2;
        HAL_ReadEntry(indexFAT + s_sectorPerSize, &buff[0]);
        retVal = GET2BYTE(&buff[indexFAT]);

    }
    else if(s_endOfFile == 0x0FFFFFFF)
    {
        indexFAT = firstCluster * 4;
        HAL_ReadEntry(indexFAT + s_sectorPerSize, &buff[0]);
        retVal = GET4BYTE(&buff[indexFAT]) & 0x0FFFFFFF;
    }
    return retVal;
}

static uint16_t FATFS_Cluster_Next_12(FATFS_BootInfor_Struct_t *bootInfor, uint32_t firstCluster)
{
    uint16_t retVal = 0;
    uint32_t indexByteFAT = 0;
    char buffFAT[32]= {0};
    
    indexByteFAT = (firstCluster * 3) /2;
    HAL_ReadEntry(indexByteFAT + s_sectorPerSize, buffFAT);
    if(0 == (firstCluster % 2))
    {
        retVal= (uint8_t)buffFAT[0];
        retVal|= (((uint16_t)buffFAT[1] & 0x000F) << 8);
    }
    else
    {
        retVal = (uint8_t)buffFAT[2];
        retVal = retVal << 4;
        retVal |= (((uint16_t)buffFAT[1] & 0x00F0) >> 4);
    }
    return retVal;
}

static void FATFS_ReadCluster(FATFS_BootInfor_Struct_t *bootSector, uint32_t indexCluster)
{
    uint32_t indexSector = 0;
    uint32_t bytesRead = 0;
    uint8_t *buff;
    buff = (uint8_t*)malloc(bootSector->sectorPerCluster * bootSector->sectorSize);
    indexSector = ((indexCluster - 2) * bootSector->sectorPerCluster) + bootSector->startSectorData;
    bytesRead = HAL_ReadMultiSector(indexSector, bootSector->sectorPerCluster, buff);
    fwrite(buff, bytesRead, 1, stdout);
}

void FATFS_ReadFile(uint32_t firstCluster, FATFS_BootInfor_Struct_t *bootInfor)
{
    bool check= true;

    while(check)
    {
        if(firstCluster == s_endOfFile)
        {
            check= false;
        }
        else
        {
            FATFS_ReadCluster(bootInfor, firstCluster);
            firstCluster= FATFS_Get_NextCluster(bootInfor, firstCluster);  
        }
    }
}

static void FATFS_Read_RootEntry(uint8_t inforEntry[], FATFS_Entry_Struct_t *entry)
{
    uint8_t index;
    uint16_t DateRoot;
    uint16_t TimeRoot;
    uint32_t firstHighCluster = 0;
    uint32_t firstLowCluster  = 0;
    TimeRoot = (inforEntry[OFFSET_REVISE_LAST_TIME + 1] << 8) + inforEntry[OFFSET_REVISE_LAST_TIME];
    DateRoot = (inforEntry[OFFSET_DATE_UPDATE_LAST + 1] << 8) + inforEntry[OFFSET_DATE_UPDATE_LAST];
    entry->dayEntry   = READ_DAY(DateRoot);
    entry->monthEntry = READ_MONTH(DateRoot);
    entry->yearEntry  = READ_YEAR(DateRoot);
    entry->secondEntry = READ_SECOND(TimeRoot);
    entry->minuteEntry = READ_MINUTE(TimeRoot);
    entry->hourEntry   = READ_HOUR(TimeRoot);
    for(index = 0; index < 8; index++)
    {
        entry->fileName[index]= inforEntry[index];
    }
    entry->fileName[8]= 0;
    if(inforEntry[0x0B] == 0x10)
    {
        entry->attributesFile = false;
    }
    else
    {
        entry->attributesFile = true;
    }
     for(index = 8; index <= 10; index++)
    {
        entry->extension[index - 8] = inforEntry[index];
    }
    entry->extension[3]= 0;
    entry->fileSize= (inforEntry[OFFSET_SIZE_FILE + 3] << 24) + (inforEntry[OFFSET_SIZE_FILE + 2] << 16) + (inforEntry[OFFSET_SIZE_FILE + 1] << 8) + inforEntry[OFFSET_SIZE_FILE];
    firstHighCluster = (inforEntry[OFFSET_CLUSTER_BEGIN_BYTE_H + 1] << 8) + inforEntry[OFFSET_CLUSTER_BEGIN_BYTE_H];
    firstLowCluster  = (inforEntry[OFFSET_CLUSTER_START_BYTE_L + 1] << 8) + inforEntry[OFFSET_CLUSTER_START_BYTE_L];
    entry->firstCluster = (firstHighCluster << 16) | (firstLowCluster);
}
bool FATFS_Init(const int8_t * const pathFile, FATFS_BootInfor_Struct_t *bootInfor)
{
    uint8_t i = 0;
    bool retVal = false;
    uint8_t *buffer = NULL;
    buffer = malloc(512);
    FATFS_FAT_Version_t FATVersion = FAT_undefine_version;
    if(true == HAL_Init(pathFile))
    {
        if(512 == (HAL_ReadSector(0, buffer)))
        {
            bootInfor->sectorSize = ((*(buffer + BYTE_PER_SECTOR + 1)) << 8) + *(buffer + BYTE_PER_SECTOR);
            bootInfor->sectorPerCluster = *(buffer + SECTOR_PER_CLUSTER);
            bootInfor->sectorPerReserve = *(buffer + NUMBER_OF_SECTOR_BEFORE_FAT);
            bootInfor->numFATs = *(buffer + NUMBER_OF_FAT);
            bootInfor->numEntry = (*(buffer + NUMBER_ENTRY_OF_RDET + 1) << 8) + *(buffer + NUMBER_ENTRY_OF_RDET);
            bootInfor->totalSector = ((*(buffer + TOTAL_SECTOR_COUNT + 1) << 8) + *(buffer + TOTAL_SECTOR_COUNT));
            if(bootInfor->totalSector == 0)
            {
            	bootInfor->totalSector = ((*(buffer + TOTAL_SECTOR_COUNT_FAT32 + 3)) << 24) + ((*(buffer + TOTAL_SECTOR_COUNT_FAT32 + 2)) << 16)
		     	+ ((*(buffer + TOTAL_SECTOR_COUNT_FAT32 + 1)) << 8) + *(buffer + TOTAL_SECTOR_COUNT_FAT32);
			}
            bootInfor->sectorPerFAT   = ((*(buffer + NUMBER_SECTOR_OF_FAT + 1) << 8) + *(buffer + NUMBER_SECTOR_OF_FAT));
            if(bootInfor->sectorPerFAT == 0)
            {
            	bootInfor->sectorPerFAT = ((*(buffer + 0x24 + 1) << 24) + (*(buffer + 0x24) << 16)) + ((*(buffer + NUMBER_SECTOR_OF_FAT + 1) << 8) + *(buffer + NUMBER_SECTOR_OF_FAT));
			}
            bootInfor->startSectorFAT = bootInfor->sectorPerReserve;
            for(i = 0; i < 5; i++)
            {
                bootInfor->versionFAT[i] = *(buffer + 0x36 + i);
            }
            if(successfully == HAL_Update_SectorSize(bootInfor->sectorSize))
            {
                s_sectorPerSize = bootInfor->sectorSize;
                retVal = true;
                if(0 == bootInfor->totalSector)
                {
                    FATVersion = FAT32_version;
                    bootInfor->startSectorRoot = bootInfor->sectorPerReserve + ((bootInfor->numFATs) * (bootInfor->sectorPerFAT));
                    bootInfor->startSectorData = bootInfor->startSectorRoot;
                    s_endOfFile = 0x0FFFFFFF;
                }
                else
                {
                    if(FAT12_version == Check_versionFAT(bootInfor->versionFAT))
                    {
                        bootInfor->startSectorRoot = bootInfor->sectorPerReserve + ((bootInfor->numFATs) * (bootInfor->sectorPerFAT));//19
                        bootInfor->startSectorData = bootInfor->startSectorRoot + (bootInfor->numEntry * 32 / (bootInfor->sectorSize));
                        s_endOfFile = 0x0FFF;
                    }
                    else if(FAT16_version == Check_versionFAT(bootInfor->versionFAT))
                    {
                        bootInfor->startSectorRoot = bootInfor->sectorPerReserve + (bootInfor->numFATs * bootInfor->sectorPerFAT);
                        bootInfor->startSectorData = bootInfor->startSectorRoot + (bootInfor->numEntry * 32 / (bootInfor->sectorSize));
                        s_endOfFile = 0xFFFF;
                    }
                }
            }
            else
            {
                retVal = false;
            }
        }
    }
    return retVal;
}

FATFS_Entry_Struct_t *FATFS_ReadDir(uint32_t firstCluster, FATFS_BootInfor_Struct_t *bootInfor)
{
    FATFS_Entry_Struct_t *head = NULL;
    FATFS_Entry_Struct_t *tail = NULL;
    FATFS_Entry_Struct_t *nodeEntry = NULL;
    uint8_t inforEntry[32] = {0};
    uint16_t index_sector = 0;
    uint32_t index_bytes = 0;
    if(firstCluster == 0)
    {
        index_sector = bootInfor->startSectorRoot;
    }
    else
    {
        index_sector = bootInfor->startSectorData + (firstCluster - 2) * bootInfor->sectorPerCluster;
    }
    index_bytes = index_sector * bootInfor->sectorSize;
    while(1)
    {
        nodeEntry = (FATFS_Entry_Struct_t*)(malloc(sizeof(FATFS_Entry_Struct_t)));
        HAL_ReadEntry(index_bytes, inforEntry);
        if(inforEntry[0] == 0x00)
        {
            free(nodeEntry);
            nodeEntry= NULL;
            break;
        }
        FATFS_Read_RootEntry(&inforEntry[0], nodeEntry);
        if(inforEntry[0x0B] != 0x0F)
        {
            if(head == NULL)
            {
                head = nodeEntry;
                tail = nodeEntry;
                tail->next = NULL;
            }
            else
            {
                tail->next = (struct FATFS_Entry_Struct_t *)nodeEntry;
                tail = nodeEntry;
                tail->next = NULL;
            }
        }
        index_bytes += 32;
    }
    return head;
}

bool FATFS_DeInit(void)
{
    bool retVal = false;
    if(true == (HAL_DeInit()))
    {
        retVal = true;
    }   
    return retVal;
}
/*******************************************************************************
 * END OF FILE
 ******************************************************************************/


