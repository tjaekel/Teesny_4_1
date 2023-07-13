#ifndef __CHIP_spi_cmd__
#define __CHIP_spi_cmd__

#if 0
#define	SPI_MAX_LEN						(3* 4096 + 2)	//chip specific
#else
#define	SPI_MAX_LEN						4096
#endif

typedef struct {
	uint8_t spiTx[SPI_MAX_LEN];
	uint8_t spiRx[SPI_MAX_LEN];
	int concurFlag;
	int wasWriteBefore;
	int device;
	unsigned int inIdx;
	unsigned int endIdx;
	unsigned int chsIdx;
	unsigned long chksumTx;
	unsigned long chksumRx;
} TSPIBuffer;

extern ECMD_DEC_Status CHIP_cid(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_rreg(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_wreg(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_peek(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_poke(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_sysc(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_concur(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_rblk(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_wblk(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_noop(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_tir(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_pgqset(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_pgset(TCMD_DEC_Results *res, EResultOut out);
extern ECMD_DEC_Status CHIP_pgget(TCMD_DEC_Results *res, EResultOut out);

extern ECMD_DEC_Status CHIP_sdload(TCMD_DEC_Results *res, EResultOut out);

//used by Pico-C C-code
#ifdef __cplusplus
extern "C" {
#endif
void CHIP_decodePRI_FIFO(uint8_t *ptr, uint16_t len, EResultOut out, int par);
#ifdef __cplusplus
}
#endif

#endif
