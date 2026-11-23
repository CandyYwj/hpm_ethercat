/*
* This source file is part of the EtherCAT Slave Stack Code licensed by Beckhoff Automation GmbH & Co KG, 33415 Verl, Germany.
* The corresponding license agreement applies. This hint shall not be removed.
*/

/**
* \addtogroup digital_io digital_io
* @{
*/

/**
\file digital_ioObjects
\author ET9300Utilities.ApplicationHandler (Version 1.6.4.0) | EthercatSSC@beckhoff.com

\brief digital_io specific objects<br>
\brief NOTE : This file will be overwritten if a new object dictionary is generated!<br>
*/

#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
#define PROTO
#else
#define PROTO extern
#endif
/******************************************************************************
*                    Object 0x1600 : Output mapping 0
******************************************************************************/
/**
* \addtogroup 0x1600 0x1600 | Output mapping 0
* @{
* \brief Object 0x1600 (Output mapping 0) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7010.0<br>
* SubIndex 2 - Reference to 0x7011.0<br>
* SubIndex 3 - Reference to 0x7012.0<br>
* SubIndex 4 - Reference to 0x7013.0<br>
* SubIndex 5 - Reference to 0x7014.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1600[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7010.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7011.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7012.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7013.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7014.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1600[] = "Output mapping 0\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7010.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7011.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7012.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7013.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7014.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1600;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1600 OutputMapping00x1600
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x70100020,0x70110020,0x70120020,0x70130020,0x70140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1601 : Output mapping 1
******************************************************************************/
/**
* \addtogroup 0x1601 0x1601 | Output mapping 1
* @{
* \brief Object 0x1601 (Output mapping 1) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7110.0<br>
* SubIndex 2 - Reference to 0x7111.0<br>
* SubIndex 3 - Reference to 0x7112.0<br>
* SubIndex 4 - Reference to 0x7113.0<br>
* SubIndex 5 - Reference to 0x7114.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1601[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7110.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7111.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7112.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7113.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7114.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1601[] = "Output mapping 1\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7110.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7111.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7112.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7113.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7114.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1601;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1601 OutputMapping10x1601
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x71100020,0x71110020,0x71120020,0x71130020,0x71140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1602 : Output mapping 2
******************************************************************************/
/**
* \addtogroup 0x1602 0x1602 | Output mapping 2
* @{
* \brief Object 0x1602 (Output mapping 2) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7210.0<br>
* SubIndex 2 - Reference to 0x7211.0<br>
* SubIndex 3 - Reference to 0x7212.0<br>
* SubIndex 4 - Reference to 0x7213.0<br>
* SubIndex 5 - Reference to 0x7214.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1602[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7210.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7211.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7212.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7213.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7214.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1602[] = "Output mapping 2\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7210.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7211.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7212.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7213.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7214.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1602;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1602 OutputMapping20x1602
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x72100020,0x72110020,0x72120020,0x72130020,0x72140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1603 : Output mapping 3
******************************************************************************/
/**
* \addtogroup 0x1603 0x1603 | Output mapping 3
* @{
* \brief Object 0x1603 (Output mapping 3) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7310.0<br>
* SubIndex 2 - Reference to 0x7311.0<br>
* SubIndex 3 - Reference to 0x7312.0<br>
* SubIndex 4 - Reference to 0x7313.0<br>
* SubIndex 5 - Reference to 0x7314.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1603[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7310.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7311.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7312.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7313.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7314.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1603[] = "Output mapping 3\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7310.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7311.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7312.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7313.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7314.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1603;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1603 OutputMapping30x1603
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x73100020,0x73110020,0x73120020,0x73130020,0x73140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1604 : Output mapping 4
******************************************************************************/
/**
* \addtogroup 0x1604 0x1604 | Output mapping 4
* @{
* \brief Object 0x1604 (Output mapping 4) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7410.0<br>
* SubIndex 2 - Reference to 0x7411.0<br>
* SubIndex 3 - Reference to 0x7412.0<br>
* SubIndex 4 - Reference to 0x7413.0<br>
* SubIndex 5 - Reference to 0x7414.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1604[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7410.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7411.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7412.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7413.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7414.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1604[] = "Output mapping 4\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7410.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7411.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7412.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7413.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7414.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1604;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1604 OutputMapping40x1604
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x74100020,0x74110020,0x74120020,0x74130020,0x74140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1605 : Output mapping 5
******************************************************************************/
/**
* \addtogroup 0x1605 0x1605 | Output mapping 5
* @{
* \brief Object 0x1605 (Output mapping 5) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7510.0<br>
* SubIndex 2 - Reference to 0x7511.0<br>
* SubIndex 3 - Reference to 0x7512.0<br>
* SubIndex 4 - Reference to 0x7513.0<br>
* SubIndex 5 - Reference to 0x7514.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1605[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7510.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7511.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7512.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7513.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7514.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1605[] = "Output mapping 5\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7510.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7511.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7512.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7513.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7514.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1605;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1605 OutputMapping50x1605
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x75100020,0x75110020,0x75120020,0x75130020,0x75140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1606 : Output mapping 6
******************************************************************************/
/**
* \addtogroup 0x1606 0x1606 | Output mapping 6
* @{
* \brief Object 0x1606 (Output mapping 6) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7610.0<br>
* SubIndex 2 - Reference to 0x7611.0<br>
* SubIndex 3 - Reference to 0x7612.0<br>
* SubIndex 4 - Reference to 0x7613.0<br>
* SubIndex 5 - Reference to 0x7614.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1606[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7610.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7611.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7612.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7613.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7614.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1606[] = "Output mapping 6\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7610.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7611.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7612.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7613.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7614.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1606;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1606 OutputMapping60x1606
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x76100020,0x76110020,0x76120020,0x76130020,0x76140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1607 : Output mapping 7
******************************************************************************/
/**
* \addtogroup 0x1607 0x1607 | Output mapping 7
* @{
* \brief Object 0x1607 (Output mapping 7) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7710.0<br>
* SubIndex 2 - Reference to 0x7711.0<br>
* SubIndex 3 - Reference to 0x7712.0<br>
* SubIndex 4 - Reference to 0x7713.0<br>
* SubIndex 5 - Reference to 0x7714.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1607[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7710.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7711.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7712.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7713.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7714.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1607[] = "Output mapping 7\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7710.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7711.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7712.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7713.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7714.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1607;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1607 OutputMapping70x1607
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x77100020,0x77110020,0x77120020,0x77130020,0x77140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1608 : Output mapping 8
******************************************************************************/
/**
* \addtogroup 0x1608 0x1608 | Output mapping 8
* @{
* \brief Object 0x1608 (Output mapping 8) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7810.0<br>
* SubIndex 2 - Reference to 0x7811.0<br>
* SubIndex 3 - Reference to 0x7812.0<br>
* SubIndex 4 - Reference to 0x7813.0<br>
* SubIndex 5 - Reference to 0x7814.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1608[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7810.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7811.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7812.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7813.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7814.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1608[] = "Output mapping 8\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7810.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7811.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7812.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7813.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7814.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1608;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1608 OutputMapping80x1608
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x78100020,0x78110020,0x78120020,0x78130020,0x78140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1609 : Output mapping 9
******************************************************************************/
/**
* \addtogroup 0x1609 0x1609 | Output mapping 9
* @{
* \brief Object 0x1609 (Output mapping 9) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7910.0<br>
* SubIndex 2 - Reference to 0x7911.0<br>
* SubIndex 3 - Reference to 0x7912.0<br>
* SubIndex 4 - Reference to 0x7913.0<br>
* SubIndex 5 - Reference to 0x7914.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1609[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7910.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7911.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7912.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7913.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7914.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1609[] = "Output mapping 9\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7910.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7911.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7912.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7913.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7914.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1609;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1609 OutputMapping90x1609
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x79100020,0x79110020,0x79120020,0x79130020,0x79140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x160A : Output mapping 10
******************************************************************************/
/**
* \addtogroup 0x160A 0x160A | Output mapping 10
* @{
* \brief Object 0x160A (Output mapping 10) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7A10.0<br>
* SubIndex 2 - Reference to 0x7A11.0<br>
* SubIndex 3 - Reference to 0x7A12.0<br>
* SubIndex 4 - Reference to 0x7A13.0<br>
* SubIndex 5 - Reference to 0x7A14.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x160A[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7A10.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7A11.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7A12.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7A13.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7A14.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x160A[] = "Output mapping 10\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7A10.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7A11.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7A12.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7A13.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7A14.0 */
} OBJ_STRUCT_PACKED_END
TOBJ160A;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ160A OutputMapping100x160A
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x7A100020,0x7A110020,0x7A120020,0x7A130020,0x7A140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x160B : Output mapping 11
******************************************************************************/
/**
* \addtogroup 0x160B 0x160B | Output mapping 11
* @{
* \brief Object 0x160B (Output mapping 11) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7B10.0<br>
* SubIndex 2 - Reference to 0x7B11.0<br>
* SubIndex 3 - Reference to 0x7B12.0<br>
* SubIndex 4 - Reference to 0x7B13.0<br>
* SubIndex 5 - Reference to 0x7B14.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x160B[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7B10.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7B11.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7B12.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7B13.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7B14.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x160B[] = "Output mapping 11\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7B10.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7B11.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7B12.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7B13.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7B14.0 */
} OBJ_STRUCT_PACKED_END
TOBJ160B;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ160B OutputMapping110x160B
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x7B100020,0x7B110020,0x7B120020,0x7B130020,0x7B140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x160C : Output mapping 12
******************************************************************************/
/**
* \addtogroup 0x160C 0x160C | Output mapping 12
* @{
* \brief Object 0x160C (Output mapping 12) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7C10.0<br>
* SubIndex 2 - Reference to 0x7C11.0<br>
* SubIndex 3 - Reference to 0x7C12.0<br>
* SubIndex 4 - Reference to 0x7C13.0<br>
* SubIndex 5 - Reference to 0x7C14.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x160C[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7C10.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7C11.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7C12.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7C13.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7C14.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x160C[] = "Output mapping 12\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7C10.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7C11.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7C12.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7C13.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7C14.0 */
} OBJ_STRUCT_PACKED_END
TOBJ160C;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ160C OutputMapping120x160C
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x7C100020,0x7C110020,0x7C120020,0x7C130020,0x7C140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x160D : Output mapping 13
******************************************************************************/
/**
* \addtogroup 0x160D 0x160D | Output mapping 13
* @{
* \brief Object 0x160D (Output mapping 13) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7D10.0<br>
* SubIndex 2 - Reference to 0x7D11.0<br>
* SubIndex 3 - Reference to 0x7D12.0<br>
* SubIndex 4 - Reference to 0x7D13.0<br>
* SubIndex 5 - Reference to 0x7D14.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x160D[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7D10.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7D11.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7D12.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7D13.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7D14.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x160D[] = "Output mapping 13\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7D10.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7D11.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7D12.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7D13.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7D14.0 */
} OBJ_STRUCT_PACKED_END
TOBJ160D;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ160D OutputMapping130x160D
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x7D100020,0x7D110020,0x7D120020,0x7D130020,0x7D140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x160E : Output mapping 14
******************************************************************************/
/**
* \addtogroup 0x160E 0x160E | Output mapping 14
* @{
* \brief Object 0x160E (Output mapping 14) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7E10.0<br>
* SubIndex 2 - Reference to 0x7E11.0<br>
* SubIndex 3 - Reference to 0x7E12.0<br>
* SubIndex 4 - Reference to 0x7E13.0<br>
* SubIndex 5 - Reference to 0x7E14.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x160E[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7E10.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7E11.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7E12.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7E13.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex5 - Reference to 0x7E14.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x160E[] = "Output mapping 14\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7E10.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7E11.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7E12.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7E13.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7E14.0 */
} OBJ_STRUCT_PACKED_END
TOBJ160E;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ160E OutputMapping140x160E
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={5,0x7E100020,0x7E110020,0x7E120020,0x7E130020,0x7E140020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x160F : Output mapping 15
******************************************************************************/
/**
* \addtogroup 0x160F 0x160F | Output mapping 15
* @{
* \brief Object 0x160F (Output mapping 15) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x7F10.0<br>
* SubIndex 2 - Reference to 0x7F11.0<br>
* SubIndex 3 - Reference to 0x7F12.0<br>
* SubIndex 4 - Reference to 0x7F13.0<br>
* SubIndex 5 - Reference to 0x7F14.0<br>
* SubIndex 6 - Reference to 0x7F66.0<br>
* SubIndex 7 - Reference to 0x7F67.0<br>
* SubIndex 8 - Reference to 0x7F68.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x160F[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x7F10.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x7F11.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x7F12.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x7F13.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex5 - Reference to 0x7F14.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex6 - Reference to 0x7F66.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex7 - Reference to 0x7F67.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex8 - Reference to 0x7F68.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x160F[] = "Output mapping 15\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000"
"SubIndex 006\000"
"SubIndex 007\000"
"SubIndex 008\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x7F10.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x7F11.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x7F12.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x7F13.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x7F14.0 */
UINT32 SI6; /* Subindex6 - Reference to 0x7F66.0 */
UINT32 SI7; /* Subindex7 - Reference to 0x7F67.0 */
UINT32 SI8; /* Subindex8 - Reference to 0x7F68.0 */
} OBJ_STRUCT_PACKED_END
TOBJ160F;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ160F OutputMapping150x160F
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={8,0x7F100020,0x7F110020,0x7F120020,0x7F130020,0x7F140020,0x7F660020,0x7F670020,0x7F680020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A00 : Input mapping 0
******************************************************************************/
/**
* \addtogroup 0x1A00 0x1A00 | Input mapping 0
* @{
* \brief Object 0x1A00 (Input mapping 0) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x3000.0<br>
* SubIndex 2 - Reference to 0x3001.0<br>
* SubIndex 3 - Reference to 0x3002.0<br>
* SubIndex 4 - Reference to 0x3003.0<br>
* SubIndex 5 - Reference to 0x3004.0<br>
* SubIndex 6 - Reference to 0x3005.0<br>
* SubIndex 7 - Reference to 0x3006.0<br>
* SubIndex 8 - Reference to 0x3007.0<br>
* SubIndex 9 - Reference to 0x3008.0<br>
* SubIndex 10 - Reference to 0x3009.0<br>
* SubIndex 11 - Reference to 0x3010.0<br>
* SubIndex 12 - Reference to 0x3011.0<br>
* SubIndex 13 - Reference to 0x3012.0<br>
* SubIndex 14 - Reference to 0x3013.0<br>
* SubIndex 15 - Reference to 0x3014.0<br>
* SubIndex 16 - Reference to 0x6000.0<br>
* SubIndex 17 - Reference to 0x6001.0<br>
* SubIndex 18 - Reference to 0x6002.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A00[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x3000.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x3001.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex3 - Reference to 0x3002.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex4 - Reference to 0x3003.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex5 - Reference to 0x3004.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex6 - Reference to 0x3005.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex7 - Reference to 0x3006.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex8 - Reference to 0x3007.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex9 - Reference to 0x3008.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex10 - Reference to 0x3009.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex11 - Reference to 0x3010.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex12 - Reference to 0x3011.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex13 - Reference to 0x3012.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex14 - Reference to 0x3013.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex15 - Reference to 0x3014.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex16 - Reference to 0x6000.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex17 - Reference to 0x6001.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex18 - Reference to 0x6002.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A00[] = "Input mapping 0\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000"
"SubIndex 004\000"
"SubIndex 005\000"
"SubIndex 006\000"
"SubIndex 007\000"
"SubIndex 008\000"
"SubIndex 009\000"
"SubIndex 010\000"
"SubIndex 011\000"
"SubIndex 012\000"
"SubIndex 013\000"
"SubIndex 014\000"
"SubIndex 015\000"
"SubIndex 016\000"
"SubIndex 017\000"
"SubIndex 018\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x3000.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x3001.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x3002.0 */
UINT32 SI4; /* Subindex4 - Reference to 0x3003.0 */
UINT32 SI5; /* Subindex5 - Reference to 0x3004.0 */
UINT32 SI6; /* Subindex6 - Reference to 0x3005.0 */
UINT32 SI7; /* Subindex7 - Reference to 0x3006.0 */
UINT32 SI8; /* Subindex8 - Reference to 0x3007.0 */
UINT32 SI9; /* Subindex9 - Reference to 0x3008.0 */
UINT32 SI10; /* Subindex10 - Reference to 0x3009.0 */
UINT32 SI11; /* Subindex11 - Reference to 0x3010.0 */
UINT32 SI12; /* Subindex12 - Reference to 0x3011.0 */
UINT32 SI13; /* Subindex13 - Reference to 0x3012.0 */
UINT32 SI14; /* Subindex14 - Reference to 0x3013.0 */
UINT32 SI15; /* Subindex15 - Reference to 0x3014.0 */
UINT32 SI16; /* Subindex16 - Reference to 0x6000.0 */
UINT32 SI17; /* Subindex17 - Reference to 0x6001.0 */
UINT32 SI18; /* Subindex18 - Reference to 0x6002.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A00;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A00 InputMapping00x1A00
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={18,0x30000020,0x30010020,0x30020020,0x30030020,0x30040020,0x30050020,0x30060020,0x30070020,0x30080020,0x30090020,0x30100020,0x30110020,0x30120020,0x30130020,0x30140020,0x60000020,0x60010020,0x60020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A01 : Input mapping 1
******************************************************************************/
/**
* \addtogroup 0x1A01 0x1A01 | Input mapping 1
* @{
* \brief Object 0x1A01 (Input mapping 1) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6100.0<br>
* SubIndex 2 - Reference to 0x6101.0<br>
* SubIndex 3 - Reference to 0x6102.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A01[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6100.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6101.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6102.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A01[] = "Input mapping 1\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6100.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6101.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6102.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A01;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A01 InputMapping10x1A01
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x61000020,0x61010020,0x61020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A02 : Input mapping 2
******************************************************************************/
/**
* \addtogroup 0x1A02 0x1A02 | Input mapping 2
* @{
* \brief Object 0x1A02 (Input mapping 2) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6200.0<br>
* SubIndex 2 - Reference to 0x6201.0<br>
* SubIndex 3 - Reference to 0x6202.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A02[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6200.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6201.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6202.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A02[] = "Input mapping 2\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6200.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6201.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6202.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A02;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A02 InputMapping20x1A02
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x62000020,0x62010020,0x62020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A03 : Input mapping 3
******************************************************************************/
/**
* \addtogroup 0x1A03 0x1A03 | Input mapping 3
* @{
* \brief Object 0x1A03 (Input mapping 3) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6300.0<br>
* SubIndex 2 - Reference to 0x6301.0<br>
* SubIndex 3 - Reference to 0x6302.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A03[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6300.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6301.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6302.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A03[] = "Input mapping 3\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6300.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6301.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6302.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A03;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A03 InputMapping30x1A03
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x63000020,0x63010020,0x63020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A04 : Input mapping 4
******************************************************************************/
/**
* \addtogroup 0x1A04 0x1A04 | Input mapping 4
* @{
* \brief Object 0x1A04 (Input mapping 4) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6400.0<br>
* SubIndex 2 - Reference to 0x6401.0<br>
* SubIndex 3 - Reference to 0x6402.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A04[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6400.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6401.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6402.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A04[] = "Input mapping 4\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6400.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6401.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6402.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A04;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A04 InputMapping40x1A04
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x64000020,0x64010020,0x64020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A05 : Input mapping 5
******************************************************************************/
/**
* \addtogroup 0x1A05 0x1A05 | Input mapping 5
* @{
* \brief Object 0x1A05 (Input mapping 5) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6500.0<br>
* SubIndex 2 - Reference to 0x6501.0<br>
* SubIndex 3 - Reference to 0x6502.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A05[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6500.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6501.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6502.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A05[] = "Input mapping 5\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6500.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6501.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6502.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A05;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A05 InputMapping50x1A05
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x65000020,0x65010020,0x65020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A06 : Input mapping 6
******************************************************************************/
/**
* \addtogroup 0x1A06 0x1A06 | Input mapping 6
* @{
* \brief Object 0x1A06 (Input mapping 6) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6600.0<br>
* SubIndex 2 - Reference to 0x6601.0<br>
* SubIndex 3 - Reference to 0x6602.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A06[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6600.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6601.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6602.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A06[] = "Input mapping 6\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6600.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6601.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6602.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A06;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A06 InputMapping60x1A06
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x66000020,0x66010020,0x66020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A07 : Input mapping 7
******************************************************************************/
/**
* \addtogroup 0x1A07 0x1A07 | Input mapping 7
* @{
* \brief Object 0x1A07 (Input mapping 7) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6700.0<br>
* SubIndex 2 - Reference to 0x6701.0<br>
* SubIndex 3 - Reference to 0x6702.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A07[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6700.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6701.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6702.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A07[] = "Input mapping 7\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6700.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6701.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6702.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A07;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A07 InputMapping70x1A07
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x67000020,0x67010020,0x67020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A08 : Input mapping 8
******************************************************************************/
/**
* \addtogroup 0x1A08 0x1A08 | Input mapping 8
* @{
* \brief Object 0x1A08 (Input mapping 8) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6800.0<br>
* SubIndex 2 - Reference to 0x6801.0<br>
* SubIndex 3 - Reference to 0x6802.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A08[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6800.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6801.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6802.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A08[] = "Input mapping 8\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6800.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6801.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6802.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A08;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A08 InputMapping80x1A08
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x68000020,0x68010020,0x68020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A09 : Input mapping 9
******************************************************************************/
/**
* \addtogroup 0x1A09 0x1A09 | Input mapping 9
* @{
* \brief Object 0x1A09 (Input mapping 9) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6900.0<br>
* SubIndex 2 - Reference to 0x6901.0<br>
* SubIndex 3 - Reference to 0x6902.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A09[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6900.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6901.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6902.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A09[] = "Input mapping 9\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6900.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6901.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6902.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A09;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A09 InputMapping90x1A09
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x69000020,0x69010020,0x69020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A0A : Input mapping 10
******************************************************************************/
/**
* \addtogroup 0x1A0A 0x1A0A | Input mapping 10
* @{
* \brief Object 0x1A0A (Input mapping 10) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6A00.0<br>
* SubIndex 2 - Reference to 0x6A01.0<br>
* SubIndex 3 - Reference to 0x6A02.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A0A[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6A00.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6A01.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6A02.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A0A[] = "Input mapping 10\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6A00.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6A01.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6A02.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A0A;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A0A InputMapping100x1A0A
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x6A000020,0x6A010020,0x6A020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A0B : Input mapping 11
******************************************************************************/
/**
* \addtogroup 0x1A0B 0x1A0B | Input mapping 11
* @{
* \brief Object 0x1A0B (Input mapping 11) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6B00.0<br>
* SubIndex 2 - Reference to 0x6B01.0<br>
* SubIndex 3 - Reference to 0x6B02.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A0B[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6B00.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6B01.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6B02.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A0B[] = "Input mapping 11\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6B00.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6B01.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6B02.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A0B;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A0B InputMapping110x1A0B
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x6B000020,0x6B010020,0x6B020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A0C : Input mapping 12
******************************************************************************/
/**
* \addtogroup 0x1A0C 0x1A0C | Input mapping 12
* @{
* \brief Object 0x1A0C (Input mapping 12) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6C00.0<br>
* SubIndex 2 - Reference to 0x6C01.0<br>
* SubIndex 3 - Reference to 0x6C02.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A0C[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6C00.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6C01.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6C02.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A0C[] = "Input mapping 12\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6C00.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6C01.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6C02.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A0C;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A0C InputMapping120x1A0C
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x6C000020,0x6C010020,0x6C020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A0D : Input mapping 13
******************************************************************************/
/**
* \addtogroup 0x1A0D 0x1A0D | Input mapping 13
* @{
* \brief Object 0x1A0D (Input mapping 13) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6D00.0<br>
* SubIndex 2 - Reference to 0x6D01.0<br>
* SubIndex 3 - Reference to 0x6D02.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A0D[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6D00.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6D01.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6D02.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A0D[] = "Input mapping 13\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6D00.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6D01.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6D02.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A0D;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A0D InputMapping130x1A0D
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x6D000020,0x6D010020,0x6D020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A0E : Input mapping 14
******************************************************************************/
/**
* \addtogroup 0x1A0E 0x1A0E | Input mapping 14
* @{
* \brief Object 0x1A0E (Input mapping 14) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6E00.0<br>
* SubIndex 2 - Reference to 0x6E01.0<br>
* SubIndex 3 - Reference to 0x6E02.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A0E[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6E00.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6E01.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6E02.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A0E[] = "Input mapping 14\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6E00.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6E01.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6E02.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A0E;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A0E InputMapping140x1A0E
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x6E000020,0x6E010020,0x6E020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1A0F : Input mapping 15
******************************************************************************/
/**
* \addtogroup 0x1A0F 0x1A0F | Input mapping 15
* @{
* \brief Object 0x1A0F (Input mapping 15) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Reference to 0x6F00.0<br>
* SubIndex 2 - Reference to 0x6F01.0<br>
* SubIndex 3 - Reference to 0x6F02.0<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1A0F[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex1 - Reference to 0x6F00.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }, /* Subindex2 - Reference to 0x6F01.0 */
{ DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ }}; /* Subindex3 - Reference to 0x6F02.0 */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0x1A0F[] = "Input mapping 15\000"
"SubIndex 001\000"
"SubIndex 002\000"
"SubIndex 003\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT32 SI1; /* Subindex1 - Reference to 0x6F00.0 */
UINT32 SI2; /* Subindex2 - Reference to 0x6F01.0 */
UINT32 SI3; /* Subindex3 - Reference to 0x6F02.0 */
} OBJ_STRUCT_PACKED_END
TOBJ1A0F;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1A0F InputMapping150x1A0F
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={3,0x6F000020,0x6F010020,0x6F020020}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1C12 : SyncManager 2 assignment
******************************************************************************/
/**
* \addtogroup 0x1C12 0x1C12 | SyncManager 2 assignment
* @{
* \brief Object 0x1C12 (SyncManager 2 assignment) definition
*/
#ifdef _OBJD_
/**
* \brief Entry descriptions<br>
* 
* Subindex 0<br>
* Subindex 1 - n (the same entry description is used)<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1C12[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED16 , 0x10 , ACCESS_READ }};

/**
* \brief Object name definition<br>
* For Subindex 1 to n the syntax 'Subindex XXX' is used
*/
OBJCONST UCHAR OBJMEM aName0x1C12[] = "SyncManager 2 assignment\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16   u16SubIndex0;  /**< \brief Subindex 0 */
UINT16 aEntries[16];  /**< \brief Subindex 1 - 16 */
} OBJ_STRUCT_PACKED_END
TOBJ1C12;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1C12 sRxPDOassign
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={16,{0x1600,0x1601,0x1602,0x1603,0x1604,0x1605,0x1606,0x1607,0x1608,0x1609,0x160A,0x160B,0x160C,0x160D,0x160E,0x160F}}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x1C13 : SyncManager 3 assignment
******************************************************************************/
/**
* \addtogroup 0x1C13 0x1C13 | SyncManager 3 assignment
* @{
* \brief Object 0x1C13 (SyncManager 3 assignment) definition
*/
#ifdef _OBJD_
/**
* \brief Entry descriptions<br>
* 
* Subindex 0<br>
* Subindex 1 - n (the same entry description is used)<br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0x1C13[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED16 , 0x10 , ACCESS_READ }};

/**
* \brief Object name definition<br>
* For Subindex 1 to n the syntax 'Subindex XXX' is used
*/
OBJCONST UCHAR OBJMEM aName0x1C13[] = "SyncManager 3 assignment\000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16   u16SubIndex0;  /**< \brief Subindex 0 */
UINT16 aEntries[16];  /**< \brief Subindex 1 - 16 */
} OBJ_STRUCT_PACKED_END
TOBJ1C13;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJ1C13 sTxPDOassign
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={16,{0x1A00,0x1A01,0x1A02,0x1A03,0x1A04,0x1A05,0x1A06,0x1A07,0x1A08,0x1A09,0x1A0A,0x1A0B,0x1A0C,0x1A0D,0x1A0E,0x1A0F}}
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3000 : ImuStatus
******************************************************************************/
/**
* \addtogroup 0x3000 0x3000 | ImuStatus
* @{
* \brief Object 0x3000 (ImuStatus) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3000 = { DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3000[] = "ImuStatus\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO UINT32 ImuStatus0x3000
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0x00000000
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3001 : ImuValidFields
******************************************************************************/
/**
* \addtogroup 0x3001 0x3001 | ImuValidFields
* @{
* \brief Object 0x3001 (ImuValidFields) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3001 = { DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3001[] = "ImuValidFields\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO UINT32 ImuValidFields0x3001
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0x00000000
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3002 : ImuReceiveTick
******************************************************************************/
/**
* \addtogroup 0x3002 0x3002 | ImuReceiveTick
* @{
* \brief Object 0x3002 (ImuReceiveTick) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3002 = { DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3002[] = "ImuReceiveTick\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO UINT32 ImuReceiveTick0x3002
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0x00000000
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3003 : ImuSampleTimeUs
******************************************************************************/
/**
* \addtogroup 0x3003 0x3003 | ImuSampleTimeUs
* @{
* \brief Object 0x3003 (ImuSampleTimeUs) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3003 = { DEFTYPE_UNSIGNED32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3003[] = "ImuSampleTimeUs\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO UINT32 ImuSampleTimeUs0x3003
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0x00000000
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3004 : ImuAccelX
******************************************************************************/
/**
* \addtogroup 0x3004 0x3004 | ImuAccelX
* @{
* \brief Object 0x3004 (ImuAccelX) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3004 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3004[] = "ImuAccelX\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuAccelX0x3004
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3005 : ImuAccelY
******************************************************************************/
/**
* \addtogroup 0x3005 0x3005 | ImuAccelY
* @{
* \brief Object 0x3005 (ImuAccelY) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3005 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3005[] = "ImuAccelY\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuAccelY0x3005
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3006 : ImuAccelZ
******************************************************************************/
/**
* \addtogroup 0x3006 0x3006 | ImuAccelZ
* @{
* \brief Object 0x3006 (ImuAccelZ) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3006 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3006[] = "ImuAccelZ\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuAccelZ0x3006
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3007 : ImuGyroX
******************************************************************************/
/**
* \addtogroup 0x3007 0x3007 | ImuGyroX
* @{
* \brief Object 0x3007 (ImuGyroX) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3007 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3007[] = "ImuGyroX\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuGyroX0x3007
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3008 : ImuGyroY
******************************************************************************/
/**
* \addtogroup 0x3008 0x3008 | ImuGyroY
* @{
* \brief Object 0x3008 (ImuGyroY) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3008 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3008[] = "ImuGyroY\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuGyroY0x3008
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3009 : ImuGyroZ
******************************************************************************/
/**
* \addtogroup 0x3009 0x3009 | ImuGyroZ
* @{
* \brief Object 0x3009 (ImuGyroZ) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3009 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3009[] = "ImuGyroZ\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuGyroZ0x3009
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3010 : ImuQuaternion0
******************************************************************************/
/**
* \addtogroup 0x3010 0x3010 | ImuQuaternion0
* @{
* \brief Object 0x3010 (ImuQuaternion0) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3010 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3010[] = "ImuQuaternion0\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuQuaternion00x3010
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3011 : ImuQuaternion1
******************************************************************************/
/**
* \addtogroup 0x3011 0x3011 | ImuQuaternion1
* @{
* \brief Object 0x3011 (ImuQuaternion1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3011 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3011[] = "ImuQuaternion1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuQuaternion10x3011
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3012 : ImuQuaternion2
******************************************************************************/
/**
* \addtogroup 0x3012 0x3012 | ImuQuaternion2
* @{
* \brief Object 0x3012 (ImuQuaternion2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3012 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3012[] = "ImuQuaternion2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuQuaternion20x3012
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3013 : ImuQuaternion3
******************************************************************************/
/**
* \addtogroup 0x3013 0x3013 | ImuQuaternion3
* @{
* \brief Object 0x3013 (ImuQuaternion3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3013 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3013[] = "ImuQuaternion3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuQuaternion30x3013
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x3014 : ImuTemperature
******************************************************************************/
/**
* \addtogroup 0x3014 0x3014 | ImuTemperature
* @{
* \brief Object 0x3014 (ImuTemperature) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x3014 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x3014[] = "ImuTemperature\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ImuTemperature0x3014
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6000 : ActualPosition0
******************************************************************************/
/**
* \addtogroup 0x6000 0x6000 | ActualPosition0
* @{
* \brief Object 0x6000 (ActualPosition0) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6000 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6000[] = "ActualPosition0\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition00x6000
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6001 : ActualVelocity0
******************************************************************************/
/**
* \addtogroup 0x6001 0x6001 | ActualVelocity0
* @{
* \brief Object 0x6001 (ActualVelocity0) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6001 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6001[] = "ActualVelocity0\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity00x6001
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6002 : ActualTorque0
******************************************************************************/
/**
* \addtogroup 0x6002 0x6002 | ActualTorque0
* @{
* \brief Object 0x6002 (ActualTorque0) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6002 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6002[] = "ActualTorque0\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque00x6002
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6100 : ActualPosition1
******************************************************************************/
/**
* \addtogroup 0x6100 0x6100 | ActualPosition1
* @{
* \brief Object 0x6100 (ActualPosition1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6100 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6100[] = "ActualPosition1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition10x6100
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6101 : ActualVelocity1
******************************************************************************/
/**
* \addtogroup 0x6101 0x6101 | ActualVelocity1
* @{
* \brief Object 0x6101 (ActualVelocity1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6101 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6101[] = "ActualVelocity1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity10x6101
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6102 : ActualTorque1
******************************************************************************/
/**
* \addtogroup 0x6102 0x6102 | ActualTorque1
* @{
* \brief Object 0x6102 (ActualTorque1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6102 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6102[] = "ActualTorque1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque10x6102
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6200 : ActualPosition2
******************************************************************************/
/**
* \addtogroup 0x6200 0x6200 | ActualPosition2
* @{
* \brief Object 0x6200 (ActualPosition2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6200 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6200[] = "ActualPosition2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition20x6200
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6201 : ActualVelocity2
******************************************************************************/
/**
* \addtogroup 0x6201 0x6201 | ActualVelocity2
* @{
* \brief Object 0x6201 (ActualVelocity2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6201 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6201[] = "ActualVelocity2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity20x6201
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6202 : ActualTorque2
******************************************************************************/
/**
* \addtogroup 0x6202 0x6202 | ActualTorque2
* @{
* \brief Object 0x6202 (ActualTorque2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6202 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6202[] = "ActualTorque2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque20x6202
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6300 : ActualPosition3
******************************************************************************/
/**
* \addtogroup 0x6300 0x6300 | ActualPosition3
* @{
* \brief Object 0x6300 (ActualPosition3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6300 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6300[] = "ActualPosition3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition30x6300
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6301 : ActualVelocity3
******************************************************************************/
/**
* \addtogroup 0x6301 0x6301 | ActualVelocity3
* @{
* \brief Object 0x6301 (ActualVelocity3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6301 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6301[] = "ActualVelocity3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity30x6301
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6302 : ActualTorque3
******************************************************************************/
/**
* \addtogroup 0x6302 0x6302 | ActualTorque3
* @{
* \brief Object 0x6302 (ActualTorque3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6302 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6302[] = "ActualTorque3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque30x6302
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6400 : ActualPosition4
******************************************************************************/
/**
* \addtogroup 0x6400 0x6400 | ActualPosition4
* @{
* \brief Object 0x6400 (ActualPosition4) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6400 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6400[] = "ActualPosition4\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition40x6400
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6401 : ActualVelocity4
******************************************************************************/
/**
* \addtogroup 0x6401 0x6401 | ActualVelocity4
* @{
* \brief Object 0x6401 (ActualVelocity4) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6401 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6401[] = "ActualVelocity4\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity40x6401
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6402 : ActualTorque4
******************************************************************************/
/**
* \addtogroup 0x6402 0x6402 | ActualTorque4
* @{
* \brief Object 0x6402 (ActualTorque4) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6402 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6402[] = "ActualTorque4\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque40x6402
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6500 : ActualPosition5
******************************************************************************/
/**
* \addtogroup 0x6500 0x6500 | ActualPosition5
* @{
* \brief Object 0x6500 (ActualPosition5) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6500 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6500[] = "ActualPosition5\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition50x6500
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6501 : ActualVelocity5
******************************************************************************/
/**
* \addtogroup 0x6501 0x6501 | ActualVelocity5
* @{
* \brief Object 0x6501 (ActualVelocity5) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6501 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6501[] = "ActualVelocity5\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity50x6501
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6502 : ActualTorque5
******************************************************************************/
/**
* \addtogroup 0x6502 0x6502 | ActualTorque5
* @{
* \brief Object 0x6502 (ActualTorque5) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6502 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6502[] = "ActualTorque5\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque50x6502
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6600 : ActualPosition6
******************************************************************************/
/**
* \addtogroup 0x6600 0x6600 | ActualPosition6
* @{
* \brief Object 0x6600 (ActualPosition6) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6600 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6600[] = "ActualPosition6\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition60x6600
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6601 : ActualVelocity6
******************************************************************************/
/**
* \addtogroup 0x6601 0x6601 | ActualVelocity6
* @{
* \brief Object 0x6601 (ActualVelocity6) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6601 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6601[] = "ActualVelocity6\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity60x6601
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6602 : ActualTorque6
******************************************************************************/
/**
* \addtogroup 0x6602 0x6602 | ActualTorque6
* @{
* \brief Object 0x6602 (ActualTorque6) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6602 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6602[] = "ActualTorque6\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque60x6602
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6700 : ActualPosition7
******************************************************************************/
/**
* \addtogroup 0x6700 0x6700 | ActualPosition7
* @{
* \brief Object 0x6700 (ActualPosition7) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6700 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6700[] = "ActualPosition7\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition70x6700
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6701 : ActualVelocity7
******************************************************************************/
/**
* \addtogroup 0x6701 0x6701 | ActualVelocity7
* @{
* \brief Object 0x6701 (ActualVelocity7) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6701 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6701[] = "ActualVelocity7\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity70x6701
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6702 : ActualTorque7
******************************************************************************/
/**
* \addtogroup 0x6702 0x6702 | ActualTorque7
* @{
* \brief Object 0x6702 (ActualTorque7) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6702 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6702[] = "ActualTorque7\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque70x6702
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6800 : ActualPosition8
******************************************************************************/
/**
* \addtogroup 0x6800 0x6800 | ActualPosition8
* @{
* \brief Object 0x6800 (ActualPosition8) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6800 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6800[] = "ActualPosition8\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition80x6800
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6801 : ActualVelocity8
******************************************************************************/
/**
* \addtogroup 0x6801 0x6801 | ActualVelocity8
* @{
* \brief Object 0x6801 (ActualVelocity8) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6801 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6801[] = "ActualVelocity8\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity80x6801
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6802 : ActualTorque8
******************************************************************************/
/**
* \addtogroup 0x6802 0x6802 | ActualTorque8
* @{
* \brief Object 0x6802 (ActualTorque8) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6802 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6802[] = "ActualTorque8\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque80x6802
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6900 : ActualPosition9
******************************************************************************/
/**
* \addtogroup 0x6900 0x6900 | ActualPosition9
* @{
* \brief Object 0x6900 (ActualPosition9) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6900 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6900[] = "ActualPosition9\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPosition90x6900
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6901 : ActualVelocity9
******************************************************************************/
/**
* \addtogroup 0x6901 0x6901 | ActualVelocity9
* @{
* \brief Object 0x6901 (ActualVelocity9) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6901 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6901[] = "ActualVelocity9\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocity90x6901
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6902 : ActualTorque9
******************************************************************************/
/**
* \addtogroup 0x6902 0x6902 | ActualTorque9
* @{
* \brief Object 0x6902 (ActualTorque9) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6902 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6902[] = "ActualTorque9\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorque90x6902
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6A00 : ActualPositionA
******************************************************************************/
/**
* \addtogroup 0x6A00 0x6A00 | ActualPositionA
* @{
* \brief Object 0x6A00 (ActualPositionA) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6A00 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6A00[] = "ActualPositionA\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPositionA0x6A00
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6A01 : ActualVelocityA
******************************************************************************/
/**
* \addtogroup 0x6A01 0x6A01 | ActualVelocityA
* @{
* \brief Object 0x6A01 (ActualVelocityA) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6A01 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6A01[] = "ActualVelocityA\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocityA0x6A01
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6A02 : ActualTorqueA
******************************************************************************/
/**
* \addtogroup 0x6A02 0x6A02 | ActualTorqueA
* @{
* \brief Object 0x6A02 (ActualTorqueA) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6A02 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6A02[] = "ActualTorqueA\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorqueA0x6A02
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6B00 : ActualPositionB
******************************************************************************/
/**
* \addtogroup 0x6B00 0x6B00 | ActualPositionB
* @{
* \brief Object 0x6B00 (ActualPositionB) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6B00 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6B00[] = "ActualPositionB\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPositionB0x6B00
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6B01 : ActualVelocityB
******************************************************************************/
/**
* \addtogroup 0x6B01 0x6B01 | ActualVelocityB
* @{
* \brief Object 0x6B01 (ActualVelocityB) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6B01 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6B01[] = "ActualVelocityB\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocityB0x6B01
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6B02 : ActualTorqueB
******************************************************************************/
/**
* \addtogroup 0x6B02 0x6B02 | ActualTorqueB
* @{
* \brief Object 0x6B02 (ActualTorqueB) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6B02 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6B02[] = "ActualTorqueB\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorqueB0x6B02
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6C00 : ActualPositionC
******************************************************************************/
/**
* \addtogroup 0x6C00 0x6C00 | ActualPositionC
* @{
* \brief Object 0x6C00 (ActualPositionC) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6C00 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6C00[] = "ActualPositionC\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPositionC0x6C00
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6C01 : ActualVelocityC
******************************************************************************/
/**
* \addtogroup 0x6C01 0x6C01 | ActualVelocityC
* @{
* \brief Object 0x6C01 (ActualVelocityC) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6C01 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6C01[] = "ActualVelocityC\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocityC0x6C01
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6C02 : ActualTorqueC
******************************************************************************/
/**
* \addtogroup 0x6C02 0x6C02 | ActualTorqueC
* @{
* \brief Object 0x6C02 (ActualTorqueC) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6C02 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6C02[] = "ActualTorqueC\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorqueC0x6C02
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6D00 : ActualPositionD
******************************************************************************/
/**
* \addtogroup 0x6D00 0x6D00 | ActualPositionD
* @{
* \brief Object 0x6D00 (ActualPositionD) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6D00 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6D00[] = "ActualPositionD\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPositionD0x6D00
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6D01 : ActualVelocityD
******************************************************************************/
/**
* \addtogroup 0x6D01 0x6D01 | ActualVelocityD
* @{
* \brief Object 0x6D01 (ActualVelocityD) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6D01 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6D01[] = "ActualVelocityD\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocityD0x6D01
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6D02 : ActualTorqueD
******************************************************************************/
/**
* \addtogroup 0x6D02 0x6D02 | ActualTorqueD
* @{
* \brief Object 0x6D02 (ActualTorqueD) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6D02 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6D02[] = "ActualTorqueD\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorqueD0x6D02
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6E00 : ActualPositionE
******************************************************************************/
/**
* \addtogroup 0x6E00 0x6E00 | ActualPositionE
* @{
* \brief Object 0x6E00 (ActualPositionE) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6E00 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6E00[] = "ActualPositionE\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPositionE0x6E00
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6E01 : ActualVelocityE
******************************************************************************/
/**
* \addtogroup 0x6E01 0x6E01 | ActualVelocityE
* @{
* \brief Object 0x6E01 (ActualVelocityE) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6E01 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6E01[] = "ActualVelocityE\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocityE0x6E01
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6E02 : ActualTorqueE
******************************************************************************/
/**
* \addtogroup 0x6E02 0x6E02 | ActualTorqueE
* @{
* \brief Object 0x6E02 (ActualTorqueE) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6E02 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6E02[] = "ActualTorqueE\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorqueE0x6E02
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6F00 : ActualPositionF
******************************************************************************/
/**
* \addtogroup 0x6F00 0x6F00 | ActualPositionF
* @{
* \brief Object 0x6F00 (ActualPositionF) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6F00 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6F00[] = "ActualPositionF\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualPositionF0x6F00
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6F01 : ActualVelocityF
******************************************************************************/
/**
* \addtogroup 0x6F01 0x6F01 | ActualVelocityF
* @{
* \brief Object 0x6F01 (ActualVelocityF) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6F01 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6F01[] = "ActualVelocityF\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualVelocityF0x6F01
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x6F02 : ActualTorqueF
******************************************************************************/
/**
* \addtogroup 0x6F02 0x6F02 | ActualTorqueF
* @{
* \brief Object 0x6F02 (ActualTorqueF) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x6F02 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READ | OBJACCESS_TXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x6F02[] = "ActualTorqueF\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 ActualTorqueF0x6F02
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7010 : TargetPosition0
******************************************************************************/
/**
* \addtogroup 0x7010 0x7010 | TargetPosition0
* @{
* \brief Object 0x7010 (TargetPosition0) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7010 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7010[] = "TargetPosition0\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition00x7010
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7011 : TargetVelocity0
******************************************************************************/
/**
* \addtogroup 0x7011 0x7011 | TargetVelocity0
* @{
* \brief Object 0x7011 (TargetVelocity0) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7011 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7011[] = "TargetVelocity0\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity00x7011
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7012 : FFTorque0
******************************************************************************/
/**
* \addtogroup 0x7012 0x7012 | FFTorque0
* @{
* \brief Object 0x7012 (FFTorque0) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7012 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7012[] = "FFTorque0\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque00x7012
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7013 : Kp0
******************************************************************************/
/**
* \addtogroup 0x7013 0x7013 | Kp0
* @{
* \brief Object 0x7013 (Kp0) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7013 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7013[] = "Kp0\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp00x7013
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7014 : Kd0
******************************************************************************/
/**
* \addtogroup 0x7014 0x7014 | Kd0
* @{
* \brief Object 0x7014 (Kd0) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7014 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7014[] = "Kd0\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd00x7014
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7110 : TargetPosition1
******************************************************************************/
/**
* \addtogroup 0x7110 0x7110 | TargetPosition1
* @{
* \brief Object 0x7110 (TargetPosition1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7110 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7110[] = "TargetPosition1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition10x7110
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7111 : TargetVelocity1
******************************************************************************/
/**
* \addtogroup 0x7111 0x7111 | TargetVelocity1
* @{
* \brief Object 0x7111 (TargetVelocity1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7111 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7111[] = "TargetVelocity1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity10x7111
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7112 : FFTorque1
******************************************************************************/
/**
* \addtogroup 0x7112 0x7112 | FFTorque1
* @{
* \brief Object 0x7112 (FFTorque1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7112 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7112[] = "FFTorque1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque10x7112
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7113 : Kp1
******************************************************************************/
/**
* \addtogroup 0x7113 0x7113 | Kp1
* @{
* \brief Object 0x7113 (Kp1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7113 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7113[] = "Kp1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp10x7113
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7114 : Kd1
******************************************************************************/
/**
* \addtogroup 0x7114 0x7114 | Kd1
* @{
* \brief Object 0x7114 (Kd1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7114 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7114[] = "Kd1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd10x7114
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7210 : TargetPosition2
******************************************************************************/
/**
* \addtogroup 0x7210 0x7210 | TargetPosition2
* @{
* \brief Object 0x7210 (TargetPosition2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7210 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7210[] = "TargetPosition2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition20x7210
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7211 : TargetVelocity2
******************************************************************************/
/**
* \addtogroup 0x7211 0x7211 | TargetVelocity2
* @{
* \brief Object 0x7211 (TargetVelocity2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7211 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7211[] = "TargetVelocity2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity20x7211
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7212 : FFTorque2
******************************************************************************/
/**
* \addtogroup 0x7212 0x7212 | FFTorque2
* @{
* \brief Object 0x7212 (FFTorque2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7212 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7212[] = "FFTorque2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque20x7212
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7213 : Kp2
******************************************************************************/
/**
* \addtogroup 0x7213 0x7213 | Kp2
* @{
* \brief Object 0x7213 (Kp2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7213 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7213[] = "Kp2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp20x7213
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7214 : Kd2
******************************************************************************/
/**
* \addtogroup 0x7214 0x7214 | Kd2
* @{
* \brief Object 0x7214 (Kd2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7214 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7214[] = "Kd2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd20x7214
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7310 : TargetPosition3
******************************************************************************/
/**
* \addtogroup 0x7310 0x7310 | TargetPosition3
* @{
* \brief Object 0x7310 (TargetPosition3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7310 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7310[] = "TargetPosition3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition30x7310
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7311 : TargetVelocity3
******************************************************************************/
/**
* \addtogroup 0x7311 0x7311 | TargetVelocity3
* @{
* \brief Object 0x7311 (TargetVelocity3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7311 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7311[] = "TargetVelocity3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity30x7311
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7312 : FFTorque3
******************************************************************************/
/**
* \addtogroup 0x7312 0x7312 | FFTorque3
* @{
* \brief Object 0x7312 (FFTorque3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7312 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7312[] = "FFTorque3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque30x7312
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7313 : Kp3
******************************************************************************/
/**
* \addtogroup 0x7313 0x7313 | Kp3
* @{
* \brief Object 0x7313 (Kp3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7313 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7313[] = "Kp3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp30x7313
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7314 : Kd3
******************************************************************************/
/**
* \addtogroup 0x7314 0x7314 | Kd3
* @{
* \brief Object 0x7314 (Kd3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7314 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7314[] = "Kd3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd30x7314
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7410 : TargetPosition4
******************************************************************************/
/**
* \addtogroup 0x7410 0x7410 | TargetPosition4
* @{
* \brief Object 0x7410 (TargetPosition4) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7410 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7410[] = "TargetPosition4\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition40x7410
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7411 : TargetVelocity4
******************************************************************************/
/**
* \addtogroup 0x7411 0x7411 | TargetVelocity4
* @{
* \brief Object 0x7411 (TargetVelocity4) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7411 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7411[] = "TargetVelocity4\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity40x7411
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7412 : FFTorque4
******************************************************************************/
/**
* \addtogroup 0x7412 0x7412 | FFTorque4
* @{
* \brief Object 0x7412 (FFTorque4) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7412 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7412[] = "FFTorque4\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque40x7412
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7413 : Kp4
******************************************************************************/
/**
* \addtogroup 0x7413 0x7413 | Kp4
* @{
* \brief Object 0x7413 (Kp4) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7413 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7413[] = "Kp4\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp40x7413
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7414 : Kd4
******************************************************************************/
/**
* \addtogroup 0x7414 0x7414 | Kd4
* @{
* \brief Object 0x7414 (Kd4) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7414 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7414[] = "Kd4\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd40x7414
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7510 : TargetPosition5
******************************************************************************/
/**
* \addtogroup 0x7510 0x7510 | TargetPosition5
* @{
* \brief Object 0x7510 (TargetPosition5) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7510 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7510[] = "TargetPosition5\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition50x7510
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7511 : TargetVelocity5
******************************************************************************/
/**
* \addtogroup 0x7511 0x7511 | TargetVelocity5
* @{
* \brief Object 0x7511 (TargetVelocity5) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7511 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7511[] = "TargetVelocity5\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity50x7511
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7512 : FFTorque5
******************************************************************************/
/**
* \addtogroup 0x7512 0x7512 | FFTorque5
* @{
* \brief Object 0x7512 (FFTorque5) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7512 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7512[] = "FFTorque5\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque50x7512
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7513 : Kp5
******************************************************************************/
/**
* \addtogroup 0x7513 0x7513 | Kp5
* @{
* \brief Object 0x7513 (Kp5) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7513 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7513[] = "Kp5\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp50x7513
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7514 : Kd5
******************************************************************************/
/**
* \addtogroup 0x7514 0x7514 | Kd5
* @{
* \brief Object 0x7514 (Kd5) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7514 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7514[] = "Kd5\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd50x7514
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7610 : TargetPosition6
******************************************************************************/
/**
* \addtogroup 0x7610 0x7610 | TargetPosition6
* @{
* \brief Object 0x7610 (TargetPosition6) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7610 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7610[] = "TargetPosition6\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition60x7610
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7611 : TargetVelocity6
******************************************************************************/
/**
* \addtogroup 0x7611 0x7611 | TargetVelocity6
* @{
* \brief Object 0x7611 (TargetVelocity6) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7611 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7611[] = "TargetVelocity6\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity60x7611
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7612 : FFTorque6
******************************************************************************/
/**
* \addtogroup 0x7612 0x7612 | FFTorque6
* @{
* \brief Object 0x7612 (FFTorque6) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7612 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7612[] = "FFTorque6\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque60x7612
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7613 : Kp6
******************************************************************************/
/**
* \addtogroup 0x7613 0x7613 | Kp6
* @{
* \brief Object 0x7613 (Kp6) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7613 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7613[] = "Kp6\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp60x7613
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7614 : Kd6
******************************************************************************/
/**
* \addtogroup 0x7614 0x7614 | Kd6
* @{
* \brief Object 0x7614 (Kd6) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7614 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7614[] = "Kd6\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd60x7614
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7710 : TargetPosition7
******************************************************************************/
/**
* \addtogroup 0x7710 0x7710 | TargetPosition7
* @{
* \brief Object 0x7710 (TargetPosition7) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7710 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7710[] = "TargetPosition7\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition70x7710
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7711 : TargetVelocity7
******************************************************************************/
/**
* \addtogroup 0x7711 0x7711 | TargetVelocity7
* @{
* \brief Object 0x7711 (TargetVelocity7) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7711 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7711[] = "TargetVelocity7\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity70x7711
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7712 : FFTorque7
******************************************************************************/
/**
* \addtogroup 0x7712 0x7712 | FFTorque7
* @{
* \brief Object 0x7712 (FFTorque7) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7712 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7712[] = "FFTorque7\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque70x7712
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7713 : Kp7
******************************************************************************/
/**
* \addtogroup 0x7713 0x7713 | Kp7
* @{
* \brief Object 0x7713 (Kp7) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7713 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7713[] = "Kp7\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp70x7713
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7714 : Kd7
******************************************************************************/
/**
* \addtogroup 0x7714 0x7714 | Kd7
* @{
* \brief Object 0x7714 (Kd7) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7714 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7714[] = "Kd7\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd70x7714
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7810 : TargetPosition8
******************************************************************************/
/**
* \addtogroup 0x7810 0x7810 | TargetPosition8
* @{
* \brief Object 0x7810 (TargetPosition8) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7810 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7810[] = "TargetPosition8\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition80x7810
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7811 : TargetVelocity8
******************************************************************************/
/**
* \addtogroup 0x7811 0x7811 | TargetVelocity8
* @{
* \brief Object 0x7811 (TargetVelocity8) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7811 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7811[] = "TargetVelocity8\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity80x7811
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7812 : FFTorque8
******************************************************************************/
/**
* \addtogroup 0x7812 0x7812 | FFTorque8
* @{
* \brief Object 0x7812 (FFTorque8) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7812 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7812[] = "FFTorque8\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque80x7812
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7813 : Kp8
******************************************************************************/
/**
* \addtogroup 0x7813 0x7813 | Kp8
* @{
* \brief Object 0x7813 (Kp8) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7813 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7813[] = "Kp8\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp80x7813
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7814 : Kd8
******************************************************************************/
/**
* \addtogroup 0x7814 0x7814 | Kd8
* @{
* \brief Object 0x7814 (Kd8) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7814 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7814[] = "Kd8\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd80x7814
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7910 : TargetPosition9
******************************************************************************/
/**
* \addtogroup 0x7910 0x7910 | TargetPosition9
* @{
* \brief Object 0x7910 (TargetPosition9) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7910 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7910[] = "TargetPosition9\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPosition90x7910
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7911 : TargetVelocity9
******************************************************************************/
/**
* \addtogroup 0x7911 0x7911 | TargetVelocity9
* @{
* \brief Object 0x7911 (TargetVelocity9) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7911 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7911[] = "TargetVelocity9\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocity90x7911
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7912 : FFTorque9
******************************************************************************/
/**
* \addtogroup 0x7912 0x7912 | FFTorque9
* @{
* \brief Object 0x7912 (FFTorque9) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7912 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7912[] = "FFTorque9\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorque90x7912
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7913 : Kp9
******************************************************************************/
/**
* \addtogroup 0x7913 0x7913 | Kp9
* @{
* \brief Object 0x7913 (Kp9) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7913 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7913[] = "Kp9\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kp90x7913
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7914 : Kd9
******************************************************************************/
/**
* \addtogroup 0x7914 0x7914 | Kd9
* @{
* \brief Object 0x7914 (Kd9) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7914 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7914[] = "Kd9\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Kd90x7914
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7A10 : TargetPositionA
******************************************************************************/
/**
* \addtogroup 0x7A10 0x7A10 | TargetPositionA
* @{
* \brief Object 0x7A10 (TargetPositionA) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7A10 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7A10[] = "TargetPositionA\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPositionA0x7A10
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7A11 : TargetVelocityA
******************************************************************************/
/**
* \addtogroup 0x7A11 0x7A11 | TargetVelocityA
* @{
* \brief Object 0x7A11 (TargetVelocityA) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7A11 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7A11[] = "TargetVelocityA\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocityA0x7A11
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7A12 : FFTorqueA
******************************************************************************/
/**
* \addtogroup 0x7A12 0x7A12 | FFTorqueA
* @{
* \brief Object 0x7A12 (FFTorqueA) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7A12 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7A12[] = "FFTorqueA\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorqueA0x7A12
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7A13 : KpA
******************************************************************************/
/**
* \addtogroup 0x7A13 0x7A13 | KpA
* @{
* \brief Object 0x7A13 (KpA) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7A13 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7A13[] = "KpA\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KpA0x7A13
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7A14 : KdA
******************************************************************************/
/**
* \addtogroup 0x7A14 0x7A14 | KdA
* @{
* \brief Object 0x7A14 (KdA) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7A14 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7A14[] = "KdA\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KdA0x7A14
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7B10 : TargetPositionB
******************************************************************************/
/**
* \addtogroup 0x7B10 0x7B10 | TargetPositionB
* @{
* \brief Object 0x7B10 (TargetPositionB) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7B10 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7B10[] = "TargetPositionB\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPositionB0x7B10
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7B11 : TargetVelocityB
******************************************************************************/
/**
* \addtogroup 0x7B11 0x7B11 | TargetVelocityB
* @{
* \brief Object 0x7B11 (TargetVelocityB) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7B11 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7B11[] = "TargetVelocityB\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocityB0x7B11
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7B12 : FFTorqueB
******************************************************************************/
/**
* \addtogroup 0x7B12 0x7B12 | FFTorqueB
* @{
* \brief Object 0x7B12 (FFTorqueB) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7B12 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7B12[] = "FFTorqueB\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorqueB0x7B12
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7B13 : KpB
******************************************************************************/
/**
* \addtogroup 0x7B13 0x7B13 | KpB
* @{
* \brief Object 0x7B13 (KpB) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7B13 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7B13[] = "KpB\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KpB0x7B13
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7B14 : KdB
******************************************************************************/
/**
* \addtogroup 0x7B14 0x7B14 | KdB
* @{
* \brief Object 0x7B14 (KdB) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7B14 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7B14[] = "KdB\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KdB0x7B14
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7C10 : TargetPositionC
******************************************************************************/
/**
* \addtogroup 0x7C10 0x7C10 | TargetPositionC
* @{
* \brief Object 0x7C10 (TargetPositionC) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7C10 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7C10[] = "TargetPositionC\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPositionC0x7C10
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7C11 : TargetVelocityC
******************************************************************************/
/**
* \addtogroup 0x7C11 0x7C11 | TargetVelocityC
* @{
* \brief Object 0x7C11 (TargetVelocityC) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7C11 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7C11[] = "TargetVelocityC\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocityC0x7C11
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7C12 : FFTorqueC
******************************************************************************/
/**
* \addtogroup 0x7C12 0x7C12 | FFTorqueC
* @{
* \brief Object 0x7C12 (FFTorqueC) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7C12 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7C12[] = "FFTorqueC\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorqueC0x7C12
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7C13 : KpC
******************************************************************************/
/**
* \addtogroup 0x7C13 0x7C13 | KpC
* @{
* \brief Object 0x7C13 (KpC) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7C13 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7C13[] = "KpC\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KpC0x7C13
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7C14 : KdC
******************************************************************************/
/**
* \addtogroup 0x7C14 0x7C14 | KdC
* @{
* \brief Object 0x7C14 (KdC) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7C14 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7C14[] = "KdC\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KdC0x7C14
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7D10 : TargetPositionD
******************************************************************************/
/**
* \addtogroup 0x7D10 0x7D10 | TargetPositionD
* @{
* \brief Object 0x7D10 (TargetPositionD) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7D10 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7D10[] = "TargetPositionD\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPositionD0x7D10
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7D11 : TargetVelocityD
******************************************************************************/
/**
* \addtogroup 0x7D11 0x7D11 | TargetVelocityD
* @{
* \brief Object 0x7D11 (TargetVelocityD) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7D11 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7D11[] = "TargetVelocityD\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocityD0x7D11
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7D12 : FFTorqueD
******************************************************************************/
/**
* \addtogroup 0x7D12 0x7D12 | FFTorqueD
* @{
* \brief Object 0x7D12 (FFTorqueD) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7D12 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7D12[] = "FFTorqueD\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorqueD0x7D12
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7D13 : KpD
******************************************************************************/
/**
* \addtogroup 0x7D13 0x7D13 | KpD
* @{
* \brief Object 0x7D13 (KpD) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7D13 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7D13[] = "KpD\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KpD0x7D13
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7D14 : KdD
******************************************************************************/
/**
* \addtogroup 0x7D14 0x7D14 | KdD
* @{
* \brief Object 0x7D14 (KdD) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7D14 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7D14[] = "KdD\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KdD0x7D14
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7E10 : TargetPositionE
******************************************************************************/
/**
* \addtogroup 0x7E10 0x7E10 | TargetPositionE
* @{
* \brief Object 0x7E10 (TargetPositionE) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7E10 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7E10[] = "TargetPositionE\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPositionE0x7E10
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7E11 : TargetVelocityE
******************************************************************************/
/**
* \addtogroup 0x7E11 0x7E11 | TargetVelocityE
* @{
* \brief Object 0x7E11 (TargetVelocityE) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7E11 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7E11[] = "TargetVelocityE\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocityE0x7E11
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7E12 : FFTorqueE
******************************************************************************/
/**
* \addtogroup 0x7E12 0x7E12 | FFTorqueE
* @{
* \brief Object 0x7E12 (FFTorqueE) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7E12 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7E12[] = "FFTorqueE\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorqueE0x7E12
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7E13 : KpE
******************************************************************************/
/**
* \addtogroup 0x7E13 0x7E13 | KpE
* @{
* \brief Object 0x7E13 (KpE) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7E13 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7E13[] = "KpE\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KpE0x7E13
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7E14 : KdE
******************************************************************************/
/**
* \addtogroup 0x7E14 0x7E14 | KdE
* @{
* \brief Object 0x7E14 (KdE) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7E14 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7E14[] = "KdE\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KdE0x7E14
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7F10 : TargetPositionF
******************************************************************************/
/**
* \addtogroup 0x7F10 0x7F10 | TargetPositionF
* @{
* \brief Object 0x7F10 (TargetPositionF) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7F10 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7F10[] = "TargetPositionF\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetPositionF0x7F10
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7F11 : TargetVelocityF
******************************************************************************/
/**
* \addtogroup 0x7F11 0x7F11 | TargetVelocityF
* @{
* \brief Object 0x7F11 (TargetVelocityF) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7F11 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7F11[] = "TargetVelocityF\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 TargetVelocityF0x7F11
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7F12 : FFTorqueF
******************************************************************************/
/**
* \addtogroup 0x7F12 0x7F12 | FFTorqueF
* @{
* \brief Object 0x7F12 (FFTorqueF) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7F12 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7F12[] = "FFTorqueF\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 FFTorqueF0x7F12
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7F13 : KpF
******************************************************************************/
/**
* \addtogroup 0x7F13 0x7F13 | KpF
* @{
* \brief Object 0x7F13 (KpF) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7F13 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7F13[] = "KpF\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KpF0x7F13
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7F14 : KdF
******************************************************************************/
/**
* \addtogroup 0x7F14 0x7F14 | KdF
* @{
* \brief Object 0x7F14 (KdF) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7F14 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7F14[] = "KdF\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 KdF0x7F14
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7F66 : Test1
******************************************************************************/
/**
* \addtogroup 0x7F66 0x7F66 | Test1
* @{
* \brief Object 0x7F66 (Test1) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7F66 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7F66[] = "Test1\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Test10x7F66
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7F67 : Test2
******************************************************************************/
/**
* \addtogroup 0x7F67 0x7F67 | Test2
* @{
* \brief Object 0x7F67 (Test2) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7F67 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7F67[] = "Test2\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Test20x7F67
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0x7F68 : Test3
******************************************************************************/
/**
* \addtogroup 0x7F68 0x7F68 | Test3
* @{
* \brief Object 0x7F68 (Test3) definition
*/
#ifdef _OBJD_
/**
* \brief Entry description
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM sEntryDesc0x7F68 = { DEFTYPE_REAL32 , 0x20 , ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
/**
* \brief Object name
*/
OBJCONST UCHAR OBJMEM aName0x7F68[] = "Test3\000\377";
#endif //#ifdef _OBJD_

/**
* \brief Object variable
*/
PROTO REAL32 Test30x7F68
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
/** @}*/



/******************************************************************************
*                    Object 0xF000 : Modular Device Profile
******************************************************************************/
/**
* \addtogroup 0xF000 0xF000 | Modular Device Profile
* @{
* \brief Object 0xF000 (Modular Device Profile) definition
*/
#ifdef _OBJD_
/**
* \brief Object entry descriptions<br>
* <br>
* SubIndex 0<br>
* SubIndex 1 - Index distance <br>
* SubIndex 2 - Maximum number of modules <br>
*/
OBJCONST TSDOINFOENTRYDESC    OBJMEM asEntryDesc0xF000[] = {
{ DEFTYPE_UNSIGNED8 , 0x8 , ACCESS_READ },
{ DEFTYPE_UNSIGNED16 , 0x10 , ACCESS_READ }, /* Subindex1 - Index distance  */
{ DEFTYPE_UNSIGNED16 , 0x10 , ACCESS_READ }}; /* Subindex2 - Maximum number of modules  */

/**
* \brief Object/Entry names
*/
OBJCONST UCHAR OBJMEM aName0xF000[] = "Modular Device Profile\000"
"Index distance \000"
"Maximum number of modules \000\377";
#endif //#ifdef _OBJD_

#ifndef _DIGITAL_IO_OBJECTS_H_
/**
* \brief Object structure
*/
typedef struct OBJ_STRUCT_PACKED_START {
UINT16 u16SubIndex0;
UINT16 IndexDistance; /* Subindex1 - Index distance  */
UINT16 MaximumNumberOfModules; /* Subindex2 - Maximum number of modules  */
} OBJ_STRUCT_PACKED_END
TOBJF000;
#endif //#ifndef _DIGITAL_IO_OBJECTS_H_

/**
* \brief Object variable
*/
PROTO TOBJF000 ModularDeviceProfile0xF000
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
={2,0x0100,0x0010}
#endif
;
/** @}*/







#ifdef _OBJD_
TOBJECT    OBJMEM ApplicationObjDic[] = {
/* Object 0x1600 */
{NULL , NULL ,  0x1600 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1600 , aName0x1600 , &OutputMapping00x1600 , NULL , NULL , 0x0000 },
/* Object 0x1601 */
{NULL , NULL ,  0x1601 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1601 , aName0x1601 , &OutputMapping10x1601 , NULL , NULL , 0x0000 },
/* Object 0x1602 */
{NULL , NULL ,  0x1602 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1602 , aName0x1602 , &OutputMapping20x1602 , NULL , NULL , 0x0000 },
/* Object 0x1603 */
{NULL , NULL ,  0x1603 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1603 , aName0x1603 , &OutputMapping30x1603 , NULL , NULL , 0x0000 },
/* Object 0x1604 */
{NULL , NULL ,  0x1604 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1604 , aName0x1604 , &OutputMapping40x1604 , NULL , NULL , 0x0000 },
/* Object 0x1605 */
{NULL , NULL ,  0x1605 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1605 , aName0x1605 , &OutputMapping50x1605 , NULL , NULL , 0x0000 },
/* Object 0x1606 */
{NULL , NULL ,  0x1606 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1606 , aName0x1606 , &OutputMapping60x1606 , NULL , NULL , 0x0000 },
/* Object 0x1607 */
{NULL , NULL ,  0x1607 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1607 , aName0x1607 , &OutputMapping70x1607 , NULL , NULL , 0x0000 },
/* Object 0x1608 */
{NULL , NULL ,  0x1608 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1608 , aName0x1608 , &OutputMapping80x1608 , NULL , NULL , 0x0000 },
/* Object 0x1609 */
{NULL , NULL ,  0x1609 , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x1609 , aName0x1609 , &OutputMapping90x1609 , NULL , NULL , 0x0000 },
/* Object 0x160A */
{NULL , NULL ,  0x160A , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x160A , aName0x160A , &OutputMapping100x160A , NULL , NULL , 0x0000 },
/* Object 0x160B */
{NULL , NULL ,  0x160B , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x160B , aName0x160B , &OutputMapping110x160B , NULL , NULL , 0x0000 },
/* Object 0x160C */
{NULL , NULL ,  0x160C , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x160C , aName0x160C , &OutputMapping120x160C , NULL , NULL , 0x0000 },
/* Object 0x160D */
{NULL , NULL ,  0x160D , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x160D , aName0x160D , &OutputMapping130x160D , NULL , NULL , 0x0000 },
/* Object 0x160E */
{NULL , NULL ,  0x160E , {DEFTYPE_PDOMAPPING , 5 | (OBJCODE_REC << 8)} , asEntryDesc0x160E , aName0x160E , &OutputMapping140x160E , NULL , NULL , 0x0000 },
/* Object 0x160F */
{NULL , NULL ,  0x160F , {DEFTYPE_PDOMAPPING , 8 | (OBJCODE_REC << 8)} , asEntryDesc0x160F , aName0x160F , &OutputMapping150x160F , NULL , NULL , 0x0000 },
/* Object 0x1A00 */
{NULL , NULL ,  0x1A00 , {DEFTYPE_PDOMAPPING , 18 | (OBJCODE_REC << 8)} , asEntryDesc0x1A00 , aName0x1A00 , &InputMapping00x1A00 , NULL , NULL , 0x0000 },
/* Object 0x1A01 */
{NULL , NULL ,  0x1A01 , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A01 , aName0x1A01 , &InputMapping10x1A01 , NULL , NULL , 0x0000 },
/* Object 0x1A02 */
{NULL , NULL ,  0x1A02 , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A02 , aName0x1A02 , &InputMapping20x1A02 , NULL , NULL , 0x0000 },
/* Object 0x1A03 */
{NULL , NULL ,  0x1A03 , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A03 , aName0x1A03 , &InputMapping30x1A03 , NULL , NULL , 0x0000 },
/* Object 0x1A04 */
{NULL , NULL ,  0x1A04 , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A04 , aName0x1A04 , &InputMapping40x1A04 , NULL , NULL , 0x0000 },
/* Object 0x1A05 */
{NULL , NULL ,  0x1A05 , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A05 , aName0x1A05 , &InputMapping50x1A05 , NULL , NULL , 0x0000 },
/* Object 0x1A06 */
{NULL , NULL ,  0x1A06 , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A06 , aName0x1A06 , &InputMapping60x1A06 , NULL , NULL , 0x0000 },
/* Object 0x1A07 */
{NULL , NULL ,  0x1A07 , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A07 , aName0x1A07 , &InputMapping70x1A07 , NULL , NULL , 0x0000 },
/* Object 0x1A08 */
{NULL , NULL ,  0x1A08 , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A08 , aName0x1A08 , &InputMapping80x1A08 , NULL , NULL , 0x0000 },
/* Object 0x1A09 */
{NULL , NULL ,  0x1A09 , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A09 , aName0x1A09 , &InputMapping90x1A09 , NULL , NULL , 0x0000 },
/* Object 0x1A0A */
{NULL , NULL ,  0x1A0A , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A0A , aName0x1A0A , &InputMapping100x1A0A , NULL , NULL , 0x0000 },
/* Object 0x1A0B */
{NULL , NULL ,  0x1A0B , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A0B , aName0x1A0B , &InputMapping110x1A0B , NULL , NULL , 0x0000 },
/* Object 0x1A0C */
{NULL , NULL ,  0x1A0C , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A0C , aName0x1A0C , &InputMapping120x1A0C , NULL , NULL , 0x0000 },
/* Object 0x1A0D */
{NULL , NULL ,  0x1A0D , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A0D , aName0x1A0D , &InputMapping130x1A0D , NULL , NULL , 0x0000 },
/* Object 0x1A0E */
{NULL , NULL ,  0x1A0E , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A0E , aName0x1A0E , &InputMapping140x1A0E , NULL , NULL , 0x0000 },
/* Object 0x1A0F */
{NULL , NULL ,  0x1A0F , {DEFTYPE_PDOMAPPING , 3 | (OBJCODE_REC << 8)} , asEntryDesc0x1A0F , aName0x1A0F , &InputMapping150x1A0F , NULL , NULL , 0x0000 },
/* Object 0x1C12 */
{NULL , NULL ,  0x1C12 , {DEFTYPE_UNSIGNED16 , 16 | (OBJCODE_ARR << 8)} , asEntryDesc0x1C12 , aName0x1C12 , &sRxPDOassign , NULL , NULL , 0x0000 },
/* Object 0x1C13 */
{NULL , NULL ,  0x1C13 , {DEFTYPE_UNSIGNED16 , 16 | (OBJCODE_ARR << 8)} , asEntryDesc0x1C13 , aName0x1C13 , &sTxPDOassign , NULL , NULL , 0x0000 },
/* Object 0x3000 */
{NULL , NULL ,  0x3000 , {DEFTYPE_UNSIGNED32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3000 , aName0x3000 , &ImuStatus0x3000 , NULL , NULL , 0x0000 },
/* Object 0x3001 */
{NULL , NULL ,  0x3001 , {DEFTYPE_UNSIGNED32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3001 , aName0x3001 , &ImuValidFields0x3001 , NULL , NULL , 0x0000 },
/* Object 0x3002 */
{NULL , NULL ,  0x3002 , {DEFTYPE_UNSIGNED32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3002 , aName0x3002 , &ImuReceiveTick0x3002 , NULL , NULL , 0x0000 },
/* Object 0x3003 */
{NULL , NULL ,  0x3003 , {DEFTYPE_UNSIGNED32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3003 , aName0x3003 , &ImuSampleTimeUs0x3003 , NULL , NULL , 0x0000 },
/* Object 0x3004 */
{NULL , NULL ,  0x3004 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3004 , aName0x3004 , &ImuAccelX0x3004 , NULL , NULL , 0x0000 },
/* Object 0x3005 */
{NULL , NULL ,  0x3005 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3005 , aName0x3005 , &ImuAccelY0x3005 , NULL , NULL , 0x0000 },
/* Object 0x3006 */
{NULL , NULL ,  0x3006 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3006 , aName0x3006 , &ImuAccelZ0x3006 , NULL , NULL , 0x0000 },
/* Object 0x3007 */
{NULL , NULL ,  0x3007 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3007 , aName0x3007 , &ImuGyroX0x3007 , NULL , NULL , 0x0000 },
/* Object 0x3008 */
{NULL , NULL ,  0x3008 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3008 , aName0x3008 , &ImuGyroY0x3008 , NULL , NULL , 0x0000 },
/* Object 0x3009 */
{NULL , NULL ,  0x3009 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3009 , aName0x3009 , &ImuGyroZ0x3009 , NULL , NULL , 0x0000 },
/* Object 0x3010 */
{NULL , NULL ,  0x3010 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3010 , aName0x3010 , &ImuQuaternion00x3010 , NULL , NULL , 0x0000 },
/* Object 0x3011 */
{NULL , NULL ,  0x3011 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3011 , aName0x3011 , &ImuQuaternion10x3011 , NULL , NULL , 0x0000 },
/* Object 0x3012 */
{NULL , NULL ,  0x3012 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3012 , aName0x3012 , &ImuQuaternion20x3012 , NULL , NULL , 0x0000 },
/* Object 0x3013 */
{NULL , NULL ,  0x3013 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3013 , aName0x3013 , &ImuQuaternion30x3013 , NULL , NULL , 0x0000 },
/* Object 0x3014 */
{NULL , NULL ,  0x3014 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x3014 , aName0x3014 , &ImuTemperature0x3014 , NULL , NULL , 0x0000 },
/* Object 0x6000 */
{NULL , NULL ,  0x6000 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6000 , aName0x6000 , &ActualPosition00x6000 , NULL , NULL , 0x0000 },
/* Object 0x6001 */
{NULL , NULL ,  0x6001 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6001 , aName0x6001 , &ActualVelocity00x6001 , NULL , NULL , 0x0000 },
/* Object 0x6002 */
{NULL , NULL ,  0x6002 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6002 , aName0x6002 , &ActualTorque00x6002 , NULL , NULL , 0x0000 },
/* Object 0x6100 */
{NULL , NULL ,  0x6100 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6100 , aName0x6100 , &ActualPosition10x6100 , NULL , NULL , 0x0000 },
/* Object 0x6101 */
{NULL , NULL ,  0x6101 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6101 , aName0x6101 , &ActualVelocity10x6101 , NULL , NULL , 0x0000 },
/* Object 0x6102 */
{NULL , NULL ,  0x6102 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6102 , aName0x6102 , &ActualTorque10x6102 , NULL , NULL , 0x0000 },
/* Object 0x6200 */
{NULL , NULL ,  0x6200 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6200 , aName0x6200 , &ActualPosition20x6200 , NULL , NULL , 0x0000 },
/* Object 0x6201 */
{NULL , NULL ,  0x6201 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6201 , aName0x6201 , &ActualVelocity20x6201 , NULL , NULL , 0x0000 },
/* Object 0x6202 */
{NULL , NULL ,  0x6202 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6202 , aName0x6202 , &ActualTorque20x6202 , NULL , NULL , 0x0000 },
/* Object 0x6300 */
{NULL , NULL ,  0x6300 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6300 , aName0x6300 , &ActualPosition30x6300 , NULL , NULL , 0x0000 },
/* Object 0x6301 */
{NULL , NULL ,  0x6301 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6301 , aName0x6301 , &ActualVelocity30x6301 , NULL , NULL , 0x0000 },
/* Object 0x6302 */
{NULL , NULL ,  0x6302 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6302 , aName0x6302 , &ActualTorque30x6302 , NULL , NULL , 0x0000 },
/* Object 0x6400 */
{NULL , NULL ,  0x6400 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6400 , aName0x6400 , &ActualPosition40x6400 , NULL , NULL , 0x0000 },
/* Object 0x6401 */
{NULL , NULL ,  0x6401 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6401 , aName0x6401 , &ActualVelocity40x6401 , NULL , NULL , 0x0000 },
/* Object 0x6402 */
{NULL , NULL ,  0x6402 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6402 , aName0x6402 , &ActualTorque40x6402 , NULL , NULL , 0x0000 },
/* Object 0x6500 */
{NULL , NULL ,  0x6500 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6500 , aName0x6500 , &ActualPosition50x6500 , NULL , NULL , 0x0000 },
/* Object 0x6501 */
{NULL , NULL ,  0x6501 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6501 , aName0x6501 , &ActualVelocity50x6501 , NULL , NULL , 0x0000 },
/* Object 0x6502 */
{NULL , NULL ,  0x6502 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6502 , aName0x6502 , &ActualTorque50x6502 , NULL , NULL , 0x0000 },
/* Object 0x6600 */
{NULL , NULL ,  0x6600 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6600 , aName0x6600 , &ActualPosition60x6600 , NULL , NULL , 0x0000 },
/* Object 0x6601 */
{NULL , NULL ,  0x6601 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6601 , aName0x6601 , &ActualVelocity60x6601 , NULL , NULL , 0x0000 },
/* Object 0x6602 */
{NULL , NULL ,  0x6602 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6602 , aName0x6602 , &ActualTorque60x6602 , NULL , NULL , 0x0000 },
/* Object 0x6700 */
{NULL , NULL ,  0x6700 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6700 , aName0x6700 , &ActualPosition70x6700 , NULL , NULL , 0x0000 },
/* Object 0x6701 */
{NULL , NULL ,  0x6701 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6701 , aName0x6701 , &ActualVelocity70x6701 , NULL , NULL , 0x0000 },
/* Object 0x6702 */
{NULL , NULL ,  0x6702 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6702 , aName0x6702 , &ActualTorque70x6702 , NULL , NULL , 0x0000 },
/* Object 0x6800 */
{NULL , NULL ,  0x6800 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6800 , aName0x6800 , &ActualPosition80x6800 , NULL , NULL , 0x0000 },
/* Object 0x6801 */
{NULL , NULL ,  0x6801 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6801 , aName0x6801 , &ActualVelocity80x6801 , NULL , NULL , 0x0000 },
/* Object 0x6802 */
{NULL , NULL ,  0x6802 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6802 , aName0x6802 , &ActualTorque80x6802 , NULL , NULL , 0x0000 },
/* Object 0x6900 */
{NULL , NULL ,  0x6900 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6900 , aName0x6900 , &ActualPosition90x6900 , NULL , NULL , 0x0000 },
/* Object 0x6901 */
{NULL , NULL ,  0x6901 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6901 , aName0x6901 , &ActualVelocity90x6901 , NULL , NULL , 0x0000 },
/* Object 0x6902 */
{NULL , NULL ,  0x6902 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6902 , aName0x6902 , &ActualTorque90x6902 , NULL , NULL , 0x0000 },
/* Object 0x6A00 */
{NULL , NULL ,  0x6A00 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6A00 , aName0x6A00 , &ActualPositionA0x6A00 , NULL , NULL , 0x0000 },
/* Object 0x6A01 */
{NULL , NULL ,  0x6A01 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6A01 , aName0x6A01 , &ActualVelocityA0x6A01 , NULL , NULL , 0x0000 },
/* Object 0x6A02 */
{NULL , NULL ,  0x6A02 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6A02 , aName0x6A02 , &ActualTorqueA0x6A02 , NULL , NULL , 0x0000 },
/* Object 0x6B00 */
{NULL , NULL ,  0x6B00 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6B00 , aName0x6B00 , &ActualPositionB0x6B00 , NULL , NULL , 0x0000 },
/* Object 0x6B01 */
{NULL , NULL ,  0x6B01 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6B01 , aName0x6B01 , &ActualVelocityB0x6B01 , NULL , NULL , 0x0000 },
/* Object 0x6B02 */
{NULL , NULL ,  0x6B02 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6B02 , aName0x6B02 , &ActualTorqueB0x6B02 , NULL , NULL , 0x0000 },
/* Object 0x6C00 */
{NULL , NULL ,  0x6C00 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6C00 , aName0x6C00 , &ActualPositionC0x6C00 , NULL , NULL , 0x0000 },
/* Object 0x6C01 */
{NULL , NULL ,  0x6C01 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6C01 , aName0x6C01 , &ActualVelocityC0x6C01 , NULL , NULL , 0x0000 },
/* Object 0x6C02 */
{NULL , NULL ,  0x6C02 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6C02 , aName0x6C02 , &ActualTorqueC0x6C02 , NULL , NULL , 0x0000 },
/* Object 0x6D00 */
{NULL , NULL ,  0x6D00 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6D00 , aName0x6D00 , &ActualPositionD0x6D00 , NULL , NULL , 0x0000 },
/* Object 0x6D01 */
{NULL , NULL ,  0x6D01 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6D01 , aName0x6D01 , &ActualVelocityD0x6D01 , NULL , NULL , 0x0000 },
/* Object 0x6D02 */
{NULL , NULL ,  0x6D02 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6D02 , aName0x6D02 , &ActualTorqueD0x6D02 , NULL , NULL , 0x0000 },
/* Object 0x6E00 */
{NULL , NULL ,  0x6E00 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6E00 , aName0x6E00 , &ActualPositionE0x6E00 , NULL , NULL , 0x0000 },
/* Object 0x6E01 */
{NULL , NULL ,  0x6E01 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6E01 , aName0x6E01 , &ActualVelocityE0x6E01 , NULL , NULL , 0x0000 },
/* Object 0x6E02 */
{NULL , NULL ,  0x6E02 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6E02 , aName0x6E02 , &ActualTorqueE0x6E02 , NULL , NULL , 0x0000 },
/* Object 0x6F00 */
{NULL , NULL ,  0x6F00 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6F00 , aName0x6F00 , &ActualPositionF0x6F00 , NULL , NULL , 0x0000 },
/* Object 0x6F01 */
{NULL , NULL ,  0x6F01 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6F01 , aName0x6F01 , &ActualVelocityF0x6F01 , NULL , NULL , 0x0000 },
/* Object 0x6F02 */
{NULL , NULL ,  0x6F02 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x6F02 , aName0x6F02 , &ActualTorqueF0x6F02 , NULL , NULL , 0x0000 },
/* Object 0x7010 */
{NULL , NULL ,  0x7010 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7010 , aName0x7010 , &TargetPosition00x7010 , NULL , NULL , 0x0000 },
/* Object 0x7011 */
{NULL , NULL ,  0x7011 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7011 , aName0x7011 , &TargetVelocity00x7011 , NULL , NULL , 0x0000 },
/* Object 0x7012 */
{NULL , NULL ,  0x7012 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7012 , aName0x7012 , &FFTorque00x7012 , NULL , NULL , 0x0000 },
/* Object 0x7013 */
{NULL , NULL ,  0x7013 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7013 , aName0x7013 , &Kp00x7013 , NULL , NULL , 0x0000 },
/* Object 0x7014 */
{NULL , NULL ,  0x7014 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7014 , aName0x7014 , &Kd00x7014 , NULL , NULL , 0x0000 },
/* Object 0x7110 */
{NULL , NULL ,  0x7110 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7110 , aName0x7110 , &TargetPosition10x7110 , NULL , NULL , 0x0000 },
/* Object 0x7111 */
{NULL , NULL ,  0x7111 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7111 , aName0x7111 , &TargetVelocity10x7111 , NULL , NULL , 0x0000 },
/* Object 0x7112 */
{NULL , NULL ,  0x7112 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7112 , aName0x7112 , &FFTorque10x7112 , NULL , NULL , 0x0000 },
/* Object 0x7113 */
{NULL , NULL ,  0x7113 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7113 , aName0x7113 , &Kp10x7113 , NULL , NULL , 0x0000 },
/* Object 0x7114 */
{NULL , NULL ,  0x7114 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7114 , aName0x7114 , &Kd10x7114 , NULL , NULL , 0x0000 },
/* Object 0x7210 */
{NULL , NULL ,  0x7210 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7210 , aName0x7210 , &TargetPosition20x7210 , NULL , NULL , 0x0000 },
/* Object 0x7211 */
{NULL , NULL ,  0x7211 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7211 , aName0x7211 , &TargetVelocity20x7211 , NULL , NULL , 0x0000 },
/* Object 0x7212 */
{NULL , NULL ,  0x7212 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7212 , aName0x7212 , &FFTorque20x7212 , NULL , NULL , 0x0000 },
/* Object 0x7213 */
{NULL , NULL ,  0x7213 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7213 , aName0x7213 , &Kp20x7213 , NULL , NULL , 0x0000 },
/* Object 0x7214 */
{NULL , NULL ,  0x7214 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7214 , aName0x7214 , &Kd20x7214 , NULL , NULL , 0x0000 },
/* Object 0x7310 */
{NULL , NULL ,  0x7310 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7310 , aName0x7310 , &TargetPosition30x7310 , NULL , NULL , 0x0000 },
/* Object 0x7311 */
{NULL , NULL ,  0x7311 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7311 , aName0x7311 , &TargetVelocity30x7311 , NULL , NULL , 0x0000 },
/* Object 0x7312 */
{NULL , NULL ,  0x7312 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7312 , aName0x7312 , &FFTorque30x7312 , NULL , NULL , 0x0000 },
/* Object 0x7313 */
{NULL , NULL ,  0x7313 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7313 , aName0x7313 , &Kp30x7313 , NULL , NULL , 0x0000 },
/* Object 0x7314 */
{NULL , NULL ,  0x7314 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7314 , aName0x7314 , &Kd30x7314 , NULL , NULL , 0x0000 },
/* Object 0x7410 */
{NULL , NULL ,  0x7410 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7410 , aName0x7410 , &TargetPosition40x7410 , NULL , NULL , 0x0000 },
/* Object 0x7411 */
{NULL , NULL ,  0x7411 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7411 , aName0x7411 , &TargetVelocity40x7411 , NULL , NULL , 0x0000 },
/* Object 0x7412 */
{NULL , NULL ,  0x7412 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7412 , aName0x7412 , &FFTorque40x7412 , NULL , NULL , 0x0000 },
/* Object 0x7413 */
{NULL , NULL ,  0x7413 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7413 , aName0x7413 , &Kp40x7413 , NULL , NULL , 0x0000 },
/* Object 0x7414 */
{NULL , NULL ,  0x7414 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7414 , aName0x7414 , &Kd40x7414 , NULL , NULL , 0x0000 },
/* Object 0x7510 */
{NULL , NULL ,  0x7510 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7510 , aName0x7510 , &TargetPosition50x7510 , NULL , NULL , 0x0000 },
/* Object 0x7511 */
{NULL , NULL ,  0x7511 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7511 , aName0x7511 , &TargetVelocity50x7511 , NULL , NULL , 0x0000 },
/* Object 0x7512 */
{NULL , NULL ,  0x7512 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7512 , aName0x7512 , &FFTorque50x7512 , NULL , NULL , 0x0000 },
/* Object 0x7513 */
{NULL , NULL ,  0x7513 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7513 , aName0x7513 , &Kp50x7513 , NULL , NULL , 0x0000 },
/* Object 0x7514 */
{NULL , NULL ,  0x7514 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7514 , aName0x7514 , &Kd50x7514 , NULL , NULL , 0x0000 },
/* Object 0x7610 */
{NULL , NULL ,  0x7610 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7610 , aName0x7610 , &TargetPosition60x7610 , NULL , NULL , 0x0000 },
/* Object 0x7611 */
{NULL , NULL ,  0x7611 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7611 , aName0x7611 , &TargetVelocity60x7611 , NULL , NULL , 0x0000 },
/* Object 0x7612 */
{NULL , NULL ,  0x7612 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7612 , aName0x7612 , &FFTorque60x7612 , NULL , NULL , 0x0000 },
/* Object 0x7613 */
{NULL , NULL ,  0x7613 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7613 , aName0x7613 , &Kp60x7613 , NULL , NULL , 0x0000 },
/* Object 0x7614 */
{NULL , NULL ,  0x7614 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7614 , aName0x7614 , &Kd60x7614 , NULL , NULL , 0x0000 },
/* Object 0x7710 */
{NULL , NULL ,  0x7710 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7710 , aName0x7710 , &TargetPosition70x7710 , NULL , NULL , 0x0000 },
/* Object 0x7711 */
{NULL , NULL ,  0x7711 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7711 , aName0x7711 , &TargetVelocity70x7711 , NULL , NULL , 0x0000 },
/* Object 0x7712 */
{NULL , NULL ,  0x7712 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7712 , aName0x7712 , &FFTorque70x7712 , NULL , NULL , 0x0000 },
/* Object 0x7713 */
{NULL , NULL ,  0x7713 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7713 , aName0x7713 , &Kp70x7713 , NULL , NULL , 0x0000 },
/* Object 0x7714 */
{NULL , NULL ,  0x7714 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7714 , aName0x7714 , &Kd70x7714 , NULL , NULL , 0x0000 },
/* Object 0x7810 */
{NULL , NULL ,  0x7810 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7810 , aName0x7810 , &TargetPosition80x7810 , NULL , NULL , 0x0000 },
/* Object 0x7811 */
{NULL , NULL ,  0x7811 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7811 , aName0x7811 , &TargetVelocity80x7811 , NULL , NULL , 0x0000 },
/* Object 0x7812 */
{NULL , NULL ,  0x7812 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7812 , aName0x7812 , &FFTorque80x7812 , NULL , NULL , 0x0000 },
/* Object 0x7813 */
{NULL , NULL ,  0x7813 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7813 , aName0x7813 , &Kp80x7813 , NULL , NULL , 0x0000 },
/* Object 0x7814 */
{NULL , NULL ,  0x7814 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7814 , aName0x7814 , &Kd80x7814 , NULL , NULL , 0x0000 },
/* Object 0x7910 */
{NULL , NULL ,  0x7910 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7910 , aName0x7910 , &TargetPosition90x7910 , NULL , NULL , 0x0000 },
/* Object 0x7911 */
{NULL , NULL ,  0x7911 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7911 , aName0x7911 , &TargetVelocity90x7911 , NULL , NULL , 0x0000 },
/* Object 0x7912 */
{NULL , NULL ,  0x7912 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7912 , aName0x7912 , &FFTorque90x7912 , NULL , NULL , 0x0000 },
/* Object 0x7913 */
{NULL , NULL ,  0x7913 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7913 , aName0x7913 , &Kp90x7913 , NULL , NULL , 0x0000 },
/* Object 0x7914 */
{NULL , NULL ,  0x7914 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7914 , aName0x7914 , &Kd90x7914 , NULL , NULL , 0x0000 },
/* Object 0x7A10 */
{NULL , NULL ,  0x7A10 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7A10 , aName0x7A10 , &TargetPositionA0x7A10 , NULL , NULL , 0x0000 },
/* Object 0x7A11 */
{NULL , NULL ,  0x7A11 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7A11 , aName0x7A11 , &TargetVelocityA0x7A11 , NULL , NULL , 0x0000 },
/* Object 0x7A12 */
{NULL , NULL ,  0x7A12 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7A12 , aName0x7A12 , &FFTorqueA0x7A12 , NULL , NULL , 0x0000 },
/* Object 0x7A13 */
{NULL , NULL ,  0x7A13 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7A13 , aName0x7A13 , &KpA0x7A13 , NULL , NULL , 0x0000 },
/* Object 0x7A14 */
{NULL , NULL ,  0x7A14 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7A14 , aName0x7A14 , &KdA0x7A14 , NULL , NULL , 0x0000 },
/* Object 0x7B10 */
{NULL , NULL ,  0x7B10 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7B10 , aName0x7B10 , &TargetPositionB0x7B10 , NULL , NULL , 0x0000 },
/* Object 0x7B11 */
{NULL , NULL ,  0x7B11 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7B11 , aName0x7B11 , &TargetVelocityB0x7B11 , NULL , NULL , 0x0000 },
/* Object 0x7B12 */
{NULL , NULL ,  0x7B12 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7B12 , aName0x7B12 , &FFTorqueB0x7B12 , NULL , NULL , 0x0000 },
/* Object 0x7B13 */
{NULL , NULL ,  0x7B13 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7B13 , aName0x7B13 , &KpB0x7B13 , NULL , NULL , 0x0000 },
/* Object 0x7B14 */
{NULL , NULL ,  0x7B14 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7B14 , aName0x7B14 , &KdB0x7B14 , NULL , NULL , 0x0000 },
/* Object 0x7C10 */
{NULL , NULL ,  0x7C10 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7C10 , aName0x7C10 , &TargetPositionC0x7C10 , NULL , NULL , 0x0000 },
/* Object 0x7C11 */
{NULL , NULL ,  0x7C11 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7C11 , aName0x7C11 , &TargetVelocityC0x7C11 , NULL , NULL , 0x0000 },
/* Object 0x7C12 */
{NULL , NULL ,  0x7C12 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7C12 , aName0x7C12 , &FFTorqueC0x7C12 , NULL , NULL , 0x0000 },
/* Object 0x7C13 */
{NULL , NULL ,  0x7C13 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7C13 , aName0x7C13 , &KpC0x7C13 , NULL , NULL , 0x0000 },
/* Object 0x7C14 */
{NULL , NULL ,  0x7C14 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7C14 , aName0x7C14 , &KdC0x7C14 , NULL , NULL , 0x0000 },
/* Object 0x7D10 */
{NULL , NULL ,  0x7D10 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7D10 , aName0x7D10 , &TargetPositionD0x7D10 , NULL , NULL , 0x0000 },
/* Object 0x7D11 */
{NULL , NULL ,  0x7D11 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7D11 , aName0x7D11 , &TargetVelocityD0x7D11 , NULL , NULL , 0x0000 },
/* Object 0x7D12 */
{NULL , NULL ,  0x7D12 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7D12 , aName0x7D12 , &FFTorqueD0x7D12 , NULL , NULL , 0x0000 },
/* Object 0x7D13 */
{NULL , NULL ,  0x7D13 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7D13 , aName0x7D13 , &KpD0x7D13 , NULL , NULL , 0x0000 },
/* Object 0x7D14 */
{NULL , NULL ,  0x7D14 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7D14 , aName0x7D14 , &KdD0x7D14 , NULL , NULL , 0x0000 },
/* Object 0x7E10 */
{NULL , NULL ,  0x7E10 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7E10 , aName0x7E10 , &TargetPositionE0x7E10 , NULL , NULL , 0x0000 },
/* Object 0x7E11 */
{NULL , NULL ,  0x7E11 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7E11 , aName0x7E11 , &TargetVelocityE0x7E11 , NULL , NULL , 0x0000 },
/* Object 0x7E12 */
{NULL , NULL ,  0x7E12 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7E12 , aName0x7E12 , &FFTorqueE0x7E12 , NULL , NULL , 0x0000 },
/* Object 0x7E13 */
{NULL , NULL ,  0x7E13 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7E13 , aName0x7E13 , &KpE0x7E13 , NULL , NULL , 0x0000 },
/* Object 0x7E14 */
{NULL , NULL ,  0x7E14 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7E14 , aName0x7E14 , &KdE0x7E14 , NULL , NULL , 0x0000 },
/* Object 0x7F10 */
{NULL , NULL ,  0x7F10 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7F10 , aName0x7F10 , &TargetPositionF0x7F10 , NULL , NULL , 0x0000 },
/* Object 0x7F11 */
{NULL , NULL ,  0x7F11 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7F11 , aName0x7F11 , &TargetVelocityF0x7F11 , NULL , NULL , 0x0000 },
/* Object 0x7F12 */
{NULL , NULL ,  0x7F12 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7F12 , aName0x7F12 , &FFTorqueF0x7F12 , NULL , NULL , 0x0000 },
/* Object 0x7F13 */
{NULL , NULL ,  0x7F13 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7F13 , aName0x7F13 , &KpF0x7F13 , NULL , NULL , 0x0000 },
/* Object 0x7F14 */
{NULL , NULL ,  0x7F14 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7F14 , aName0x7F14 , &KdF0x7F14 , NULL , NULL , 0x0000 },
/* Object 0x7F66 */
{NULL , NULL ,  0x7F66 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7F66 , aName0x7F66 , &Test10x7F66 , NULL , NULL , 0x0000 },
/* Object 0x7F67 */
{NULL , NULL ,  0x7F67 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7F67 , aName0x7F67 , &Test20x7F67 , NULL , NULL , 0x0000 },
/* Object 0x7F68 */
{NULL , NULL ,  0x7F68 , {DEFTYPE_REAL32 , 0 | (OBJCODE_VAR << 8)} , &sEntryDesc0x7F68 , aName0x7F68 , &Test30x7F68 , NULL , NULL , 0x0000 },
/* Object 0xF000 */
{NULL , NULL ,  0xF000 , {DEFTYPE_RECORD , 2 | (OBJCODE_REC << 8)} , asEntryDesc0xF000 , aName0xF000 , &ModularDeviceProfile0xF000 , NULL , NULL , 0x0000 },
{NULL,NULL, 0xFFFF, {0, 0}, NULL, NULL, NULL, NULL}};
#endif    //#ifdef _OBJD_

#undef PROTO

/** @}*/
#define _DIGITAL_IO_OBJECTS_H_
