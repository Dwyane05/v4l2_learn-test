/*
 * code.h
 *
 *  Created on: Jul 21, 2016
 *      Author: xiao
 */

#ifndef YOKO_CODE_H_
#define YOKO_CODE_H_

/*
一维条码(1D)
*/
#define YOKO_CODE_25			1	//Interleaved 2 of 5	1
#define YOKO_CODE_UPC_EAN		2	//UPC/EAN	2
#define YOKO_CODE_UPC_EAN_SUP 	3 	//UPC/EAN with supplementals	3
#define YOKO_CODE_EAN 			4	//Booklan EAN	4
#define YOKO_CODE_ISSN			5	//ISSN	5
#define YOKO_CODE_UCC_CEC		6	//UCC Coupon Extended Code	6
#define YOKO_CODE_128			7	//Code 128	7
#define YOKO_CODE_GS1_128		8	//GS1-128	8
#define YOKO_CODE_ISBT128		9	//ISBT 128	9
#define YOKO_CODE_39			10	//Code 39	10
#define YOKO_CODE_39_FA			11	//Code 39 Full ASCII	11
#define YOKO_CODE_TC39			12	//Trioptic Code 39	12
#define YOKO_CODE_32			13	//Code 32	13
#define YOKO_CODE_93 			14	//Code 93	14
#define YOKO_CODE_11 			15	//Code 11	15
#define YOKO_CODE_M25 			16 	//Matrix 2 of 5	16
#define YOKO_CODE_D25 			17	//Discrete 2 of 5	17
#define YOKO_CODE_CODABAR 		18 	//Codabar	18
#define YOKO_CODE_MSI 			19	//MSI	19
#define YOKO_CODE_CHINA25 		20	//Chinese 2 of 5	20
#define YOKO_CODE_GS1DV 		21	//GS1 DataBar variants	21
#define YOKO_CODE_KOREAN35		22	//Korean 3 of 5	22
#define YOKO_CODE_ISBTCONCAT	23	//ISBT Concat	23


/*
二维条码(2D)
*/
#define YOKO_CODE_DM				51	//Data Matrix	51
#define YOKO_CODE_PDF417			52	//PDF417	52
#define YOKO_CODE_MICRO_PDF417 		53 	//MicroPDF417	53
#define YOKO_CODE_TLC39 			54  //TLC-39	54
#define YOKO_CODE_COMPOSITE 		55  //Composite Codes	55
#define YOKO_CODE_MAXI				56	// Maxicode	56
#define YOKO_CODE_QR				57	// QR Code	57
#define YOKO_CODE_MICRO_QR			58	// 	MicroQR	58
#define YOKO_CODE_AZTEC				59	// Aztec	59




#endif /* CODE_H_ */
