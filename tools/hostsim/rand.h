extern uint32_t g_extra;
static uint32_t rs=12345; static uint8_t rand(void){rs=rs*1103515245u+12345u;return (rs>>16)&0xFF;} static void initrand(uint16_t s){rs=s*7919u+1+g_extra*2654435761u;}
