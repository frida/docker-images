IMX35 SDMA LIBRARY USER'S GUIDE

The IMX35 can support up to 31 DMA channels of the following types:

TYPE 0:  Software-Driven , multi-purpose DMA

    Features:
    - Uses AP_2_AP script which can dma from any source to any destination with any alignment
    - For performances purposes, it is recommended that the src and dst addresses be 32-bit aligned
  
TYPE 1: Event Driven DMA from Memory Buffer to Peripheral FIFO

    Features:
    - supports 8,16,32 bit fifos
    - uses MCU_2_AP script
    - supports SDMA events


TYPE 2: Event Driven DMA from Peripheral Fifo to Memory Buffer

    Features:
    - supports 8,16,32 bit fifos
    - uses AP_2_MCU script
    - supports SDMA events


API OPTIONS:

init() options = irq=i,regbase=0xaaaaaaaa
 defaults: irq=34, regbase = 0x53fd4000



channel_attach() options: eventnum=e,watermark=w, fifopaddr=0xaaaaaaaa,regen,contloop
    
  Event Based DMA (type1 and type2) MUST specify an 'eventnum' [0-31], 
  a 'watermark' i.e. number of transfers per event, and a fifo physical address.
  
  The 'regen' option tells the library to automatically re-enable the descriptor ring.
  The 'contloop' tells the dma engine to continously repeat a transfer

    
Supported Attach Flags:
	DMA_ATTACH_EVENT_ON_COMPLETE =	0x00000010,	/* Want an event on transfer completion */
	DMA_ATTACH_EVENT_PER_SEGMENT =	0x00000020,	/* Want an event per fragment transfer completion */

Supported Xfer Flags:
NONE

    

TYPE 0 Example:
{
dma_functions_t sdma_funcs;
void * handle;
unsigned channel;
int priority;
struct sigevent sdmaevent;
dma_attach_flags attach_flags;

get_dmafuncs(&sdma_funcs,sizeof(dma_functions_t));

////////////////////
// init library   //
////////////////////
if ( sdma_funcs.init(NULL) !=0 ) {
    ERROR!
}


////////////////////
// create channel //
////////////////////

channel = 0; // valid range 0-2

// valid range 1-7... Everyone should be nice and use lowest priority (1).
// only use higher priority if your performance is suffering. Improved performace may/will be
// at the expense of another driver's performance.
priority = 1;  

//attach_flags=0;  // only used when polling 
//attach_flags=DMA_ATTACH_EVENT_PER_SEGMENT;  
attach_flags=DMA_ATTACH_EVENT_ON_COMPLETE;  //dma lib will throw an event after all segments have been processed

// setup an event
chid = ChannelCreate(_NTO_CHF_UNBLOCK | _NTO_CHF_DISCONNECT);
coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
sdmaevent.sigev_notify   = SIGEV_PULSE;
sdmaevent.sigev_coid     = coid;
sdmaevent.sigev_code     = 1;
sdmaevent.sigev_priority = 21;

handle = sdma_funcs.channel_attach(NULL,&sdmaevent,&channel,priority,attach_flags);
if (handle == NULL) {
    ERROR!
}

////////////////////
// SETUP the XFER //
////////////////////

// Can have up to 'max_src_fragments' fragments and 'max_xfer_size' fragment length
// Must have the same number of source and dst fragments currently.
dma_transfer.src_addrs[0].paddr = SRC_ADDR;
dma_transfer.src_addrs[0].len = fragment_length_in_bytes;
dma_transfer.dst_addrs[0].paddr = DST_ADDR;
dma_transfer.dst_addrs[0].len = fragment_length_in_bytes;

dma_transfer.xfer_unit_size = 32;  // valid values are 8,16,32

// ALL other dma_transfer parameters are ignored!


if (sdma_funcs.setup_xfer(handle,&dma_transfer) !=0) {
    ERROR
}

////////////////////
// START the XFER //
////////////////////

// start the transfer
if (sdma_funcs.xfer_start(handle,&dma_transfer) !=0) {
    ERROR
}


////////////////////
//     WAIT       // 
////////////////////

if (polling_enable) {
    while (sdma_funcs.bytes_left(handle) == 1)
} else {
    wait_for(&sdmaevent);
}



////////////////////
//  close channel //
////////////////////

sdma_funcs.channel_release(handle);


////////////////////
// close library   //
////////////////////
// do only once
sdma_funcs.fini();

}


TYPE 1 Example:

Similar to TYPE0, except on how to open the channel...

channel=1;

handle = sdma_funcs.channel_attach("eventnum=9,watermark=4,fifopaddr=0xabcdef01",&sdmaevent,&channel,priority,attach_flags);
if (handle == NULL) {
    ERROR!
}







