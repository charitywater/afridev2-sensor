AfridevV2_MSP430.out
--image
--memwidth 16
--ti_txt
--order MS

ROMS
{
	EPROM1: org = 0x9000, len = 0x5000, romwidth = 16, fill = 0xFFFF
		files = { AfridevV2_MSP430_rom.txt }
}
