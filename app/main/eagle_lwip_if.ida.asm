; ---------------------------------------------------------------------------

task_if0:                               ; DATA XREF: .irom0.text:a_task_if0o
                addi            a1, a1, 0xF0
                s32i.n          a12, a1, 4
                s32i.n          a0, a1, 0
                mov.n           a12, a2  ; struct ETSEventTag *e
                movi.n          a2, 0
                call0           eagle_lwip_getif
                l32i.n          a0, a12, 0 ; e->sig
                mov.n           a3, a2 ; struct netif *myif
                bnez.n          a0, loc_4024344D
                l32i.n          a12, a12, 4 ; e->par
                beqz.n          a2, loc_4024343E
                l32i.n          a0, a3, 0x10
                mov.n           a2, a12
                callx0          a0 ; myif->input((struct pbuf *)(e->par), myif);
                j               loc_4024344D
; ---------------------------------------------------------------------------

loc_4024343E:                           ; CODE XREF: .irom0.text:40243432j
                l32i            a2, a12, 0x10
                l32r            a0, a_ppRecycleRxPkt
                callx0          a0
                or              a2, a12, a12
                call0           pbuf_free

loc_4024344D:                           ; CODE XREF: .irom0.text:4024342Ej
                                        ; .irom0.text:4024343Bj
                l32i            a12, a1, 4
                l32i.n          a0, a1, 0
                addi            a1, a1, 0x10
                ret.n
; ---------------------------------------------------------------------------
                .byte    0
; ---------------------------------------------------------------------------

task_if1:                               ; DATA XREF: .irom0.text:a_task_if1o
                addi            a1, a1, 0xF0
                s32i.n          a12, a1, 4
                s32i.n          a0, a1, 0
                mov.n           a12, a2
                movi.n          a2, 1
                call0           eagle_lwip_getif
                l32i.n          a0, a12, 0
                mov.n           a3, a2
                bnez.n          a0, loc_40243489 ; if (e->sig == 0)
                l32i.n          a12, a12, 4 ; e->par
                beqz.n          a2, loc_4024347A
                l32i.n          a0, a3, 16
                mov.n           a2, a12
                callx0          a0
                j               loc_40243489
; ---------------------------------------------------------------------------

loc_4024347A:                           ; CODE XREF: .irom0.text:4024346Ej
                l32i            a2, a12, 16
                l32r            a0, a_ppRecycleRxPkt
                callx0          a0
                or              a2, a12, a12
                call0           pbuf_free

loc_40243489:                           ; CODE XREF: .irom0.text:4024346Aj
                                        ; .irom0.text:40243477j
                l32i            a12, a1, 4
                l32i.n          a0, a1, 0
                addi            a1, a1, 0x10
                ret.n
; ---------------------------------------------------------------------------
                .byte 0
; ---------------------------------------------------------------------------

init_fn:                                ; DATA XREF: .irom0.text:a_init_fno
                movi            a3, 0xB2
                movi            a4, 1500
                movi.n          a5, 6
                s8i             a5, a2, 42
                s16i            a4, a2, 40
                s8i             a3, a2, 49
                movi.n          a2, 0
                ret.n
; ---------------------------------------------------------------------------
                .byte 0, 0, 0
a_etharp_output .int etharp_output      ; DATA XREF: eagle_lwip_if_alloc+27r
a_ieee80211_output_pbuf .int ieee80211_output_pbuf
                                        ; DATA XREF: eagle_lwip_if_alloc+2Ar
a_lwip_if_queues .int lwip_if_queues    ; DATA XREF: eagle_lwip_if_alloc+A3r
                                        ; eagle_lwip_if_alloc+16Br ...
a_task_if1      .int task_if1           ; DATA XREF: eagle_lwip_if_alloc+AAr
a_ethernet_input .int ethernet_input    ; DATA XREF: eagle_lwip_if_alloc+BEr
                                        ; eagle_lwip_if_alloc+185r
a_init_fn       .int init_fn            ; DATA XREF: eagle_lwip_if_alloc+B5r
                                        ; eagle_lwip_if_alloc+17Dr
a_dhcps_flag    .int dhcps_flag         ; DATA XREF: eagle_lwip_if_alloc+CBr
                                        ; eagle_lwip_if_free:loc_40243694r
aaDhcpServerStar .int aDhcpServerStar   ; DATA XREF: eagle_lwip_if_alloc+D9r
                                        ; "dhcp server start:("
aaIpD_D_D_D     .int aIpD_D_D_DMas_0    ; DATA XREF: eagle_lwip_if_alloc+E2r
                                        ; "ip:%d.%d.%d.%d,mask:%d.%d.%d.%d,gw:%d.%"...
aasc_4026B6D0   .int asc_4026B6D0       ; DATA XREF: eagle_lwip_if_alloc+11Dr
                                        ; ")\n"
a_task_if0      .int task_if0           ; DATA XREF: eagle_lwip_if_alloc+172r

; =============== S U B R O U T I N E =======================================

; struct netif * ICACHE_FLASH_ATTR eagle_lwip_if_alloc(struct myif_state *state, u8_t hw[6], ip_addr_t *ips)

eagle_lwip_if_alloc:                    ; CODE XREF: wifi_softap_start+B5j
                                        ; wifi_station_start+38j
                addi            a1, a1, -0x40
                s32i.n          a14, a1, 0x3C
                s32i.n          a3, a1, 0x2C
                s32i.n          a13, a1, 0x38
                s32i.n          a12, a1, 0x34
                s32i.n          a0, a1, 0x30
                mov.n           a12, a2
                l32i.n          a0, a2, 0 ; state->myif
                mov.n           a13, a4 ; ips
                bnez.n          a0, loc_402434F9 ;  if (state->myif == NULL)
                movi.n          a2, 60
                l32r            a0, a_pvPortMalloc
                callx0          a0
                mov.n           a14, a2 ; myif
                s32i.n          a2, a12, 0 ;  state->myif = myif

loc_402434F9:                           ; CODE XREF: eagle_lwip_if_alloc+13j
                s32i            a12, a14, 28 ; myif->state = state
                l32i            a3, a1, 0x2C ; hw
                l32r            a4, a_etharp_output
                l32r            a2, a_ieee80211_output_pbuf
                movi            a5, 0x77 ; 'w'
                movi            a6, 0x65 ; 'e'
                s8i             a6, a14, 50 ; myif->name[0]
                s8i             a5, a14, 51 ; myif->name[1]
                s32i            a2, a14, 24 ; myif->linkoutput = ieee80211_output_pbuf;
                s32i.n          a4, a14, 20 ; myif->output = etharp_output;
                addi            a2, a14, 43 ; &myif->hwaddr
                movi.n          a4, 6
                l32r            a0, a_ets_memcpy_0
                callx0          a0      ; ets_memcpy(myif->hwaddr, hw, 6);
                l32i            a7, a12, 0xB0 ; state + 176 ? state - 80 ?
                bnez.n          a7, loc_40243539
                call0           wifi_station_dhcpc_status
                addi.n          a11, a2, -1
                bnez            a11, loc_4024360D ; if(wifi_station_dhcpc_status()!=1)
                movi.n          a13, 0
                s32i.n          a13, a1, 0x20 ; ip
                s32i.n          a13, a1, 0x24 ; mask
                s32i.n          a13, a1, 0x28 ; gw
                j               loc_40243637
; ---------------------------------------------------------------------------

loc_40243539:                           ; CODE XREF: eagle_lwip_if_alloc+4Cj
                addi            a2, a1, 0x20
                or              a3, a13, a13 ; ips
                movi.n          a4, 4
                l32r            a0, a_memcpy
                callx0          a0
                addi            a2, a1, 0x24 ; mask
                addi.n          a3, a13, 4
                movi.n          a4, 4
                l32r            a0, a_memcpy
                callx0          a0
                addi            a2, a1, 0x28 ; gw
                addi.n          a3, a13, 8
                movi.n          a4, 4
                l32r            a0, a_memcpy
                callx0          a0
                mov.n           a2, a14
                addi            a3, a1, 0x20
                addi            a4, a1, 0x24
                addi            a5, a1, 0x28
                call0           netif_set_addr ; netif_set_addr(myif, ips.ip, ips.mask, ips.gw)
                movi.n          a2, 80
                l32r            a0, a_pvPortMalloc
                callx0          a0      ; pvPortMalloc(80)
                movi.n          a3, 29
                movi.n          a5, 10
                l32r            a0, a_lwip_if_queues
                mov.n           a4, a2
                s32i.n          a2, a0, 4 ; lwip_if_queues[1] = queue;
                l32r            a2, a_task_if1
                l32r            a0, a_ets_task
                callx0          a0      ; ets_task(task_if1, TASK_IF1_PRIO, (ETSEvent *)queue, QUEUE_LEN);
                mov.n           a6, a12 ; state
                l32r            a7, a_init_fn
                addi            a3, a1, 0x20 ; &ipaddr
                addi            a4, a1, 0x24 ; &netmask
                l32r            a2, a_ethernet_input
                addi            a5, a1, 0x28 ; &gw
                s32i.n          a2, a1, 0 ; ethernet_input
                mov.n           a2, a14 ; myif
                call0           netif_add ; netif_add(myif, &ipaddr, &netmask, &gw, state, init_fn, ethernet_input);
                l32r            a3, a_dhcps_flag
                l8ui            a3, a3, 0
                beqz            a3, loc_402435FE
                mov.n           a2, a13
                call0           dhcps_start
                l32r            a2, aaDhcpServerStar
                l32r            a0, a_os_printf_plus
                callx0          a0
                l32r            a2, aaIpD_D_D_D
                l8ui            a7, a1, 0x24
                l8ui            a6, a1, 0x23
                l8ui            a5, a1, 0x22
                l8ui            a4, a1, 0x21
                l8ui            a3, a1, 0x20
                l8ui            a0, a1, 0x25
                s32i.n          a0, a1, 0
                l8ui            a13, a1, 0x26
                s32i.n          a13, a1, 4
                l8ui            a12, a1, 0x27
                s32i.n          a12, a1, 8
                l8ui            a11, a1, 0x28
                s32i.n          a11, a1, 0xC
                l8ui            a10, a1, 0x29
                s32i.n          a10, a1, 0x10
                l8ui            a9, a1, 0x2A
                s32i.n          a9, a1, 0x14
                l8ui            a8, a1, 0x2B
                s32i.n          a8, a1, 0x18
                l32r            a0, a_os_printf_plus
                callx0          a0
                l32r            a2, aasc_4026B6D0
                l32r            a0, a_os_printf_plus
                callx0          a0

loc_402435FE:                           ; CODE XREF: eagle_lwip_if_alloc+D1j
                                        ; eagle_lwip_if_alloc+193j
                l32i.n          a0, a1, 0x30
                l32i.n          a12, a1, 0x34
                mov.n           a2, a14 ; myif
                l32i.n          a13, a1, 0x38
                l32i.n          a14, a1, 0x3C
                addi            a1, a1, 0x40
                ret.n
; ---------------------------------------------------------------------------

loc_4024360D:                           ; CODE XREF: eagle_lwip_if_alloc+53j
                addi            a2, a1, 0x20
                or              a3, a13, a13
                movi            a4, 4
                l32r            a0, a_memcpy
                callx0          a0
                addi            a2, a1, 0x24
                addi            a3, a13, 4
                movi.n          a4, 4
                l32r            a0, a_memcpy
                callx0          a0
                addi.n          a3, a13, 8
                addi            a2, a1, 0x28
                movi.n          a4, 4
                l32r            a0, a_memcpy
                callx0          a0

loc_40243637:                           ; CODE XREF: eagle_lwip_if_alloc+5Ej
                movi.n          a2, 80
                l32r            a0, a_pvPortMalloc
                callx0          a0
                mov.n           a4, a2
                movi.n          a3, 28
                l32r            a2, a_lwip_if_queues
                movi.n          a5, 10
                s32i.n          a4, a2, 0
                l32r            a2, a_task_if0
                l32r            a0, a_ets_task
                callx0          a0      ; ets_task(task_if0, TASK_IF0_PRIO, (ETSEvent *)queue, QUEUE_LEN);
                mov.n           a6, a12
                l32r            a7, a_init_fn
                mov.n           a2, a14
                addi            a4, a1, 0x24
                l32r            a3, a_ethernet_input
                addi            a5, a1, 0x28
                s32i.n          a3, a1, 0
                addi            a3, a1, 0x20
                call0           netif_add ; netif_add(myif, &ipaddr, &netmask, &gw, state, init_fn, ethernet_input);
                j               loc_402435FE
; End of function eagle_lwip_if_alloc

; ---------------------------------------------------------------------------
                .short 0

; =============== S U B R O U T I N E =======================================


eagle_lwip_if_free:                     ; CODE XREF: wifi_station_disconnect+7Ej
                                        ; wifi_softap_stop+7Ej
                addi            a1, a1, 0xF0
                s32i            a12, a1, 4
                or              a12, a2, a2
                l32i            a2, a2, 176 ; state->dhcps_if
                s32i            a0, a1, 0
                bnez.n          a2, loc_40243694
                l32i.n          a2, a12, 0
                call0           netif_remove    ; netif_remove(state->myif)
                l32r            a2, a_lwip_if_queues
                l32i.n          a2, a2, 0
                l32r            a0, a_vPortFree
                callx0          a0 ; vPortFree(lwip_if_queues[0]);
                j               loc_402436B4
; ---------------------------------------------------------------------------

loc_40243694:                           ; CODE XREF: eagle_lwip_if_free+Fj
                l32r            a3, a_dhcps_flag
                l8ui            a3, a3, 0
                beqz.n          a3, loc_402436A2
                or              a1, a1, a1
                call0           dhcps_stop ; dhcps_stop();

loc_402436A2:                           ; CODE XREF: eagle_lwip_if_free+2Aj
                l32i            a2, a12, 0
                call0           netif_remove ; netif_remove(state->myif);
                l32r            a2, a_lwip_if_queues
                l32i            a2, a2, 4
                l32r            a0, a_vPortFree
                callx0          a0 ; vPortFree(lwip_if_queues[1]);

loc_402436B4:                           ; CODE XREF: eagle_lwip_if_free+21j
                l32i            a2, a12, 0
                beqz.n          a2, loc_402436C3
                l32r            a0, a_vPortFree
                callx0          a0 ; vPortFree(state->myif);
                movi.n          a3, 0
                s32i.n          a3, a12, 0

loc_402436C3:                           ; CODE XREF: eagle_lwip_if_free+47j
                l32i.n          a12, a1, 4
                l32i.n          a0, a1, 0
                addi            a1, a1, 0x10
                ret.n
; End of function eagle_lwip_if_free


; =============== S U B R O U T I N E =======================================


eagle_lwip_getif:                       ; CODE XREF: wifi_softap_dhcps_start+19j
                                        ; wifi_softap_dhcps_stop+1Aj ...
                l32r            a4, a_g_ic
                bnez.n          a2, loc_402436D9
                l32i.n          a2, a4, 0x10 ; g_ic.g.netif1;
                bnez.n          a2, loc_402436EA
                movi.n          a2, 0
                ret.n
; ---------------------------------------------------------------------------

loc_402436D9:                           ; CODE XREF: eagle_lwip_getif+3j
                bnei            a2, 1, loc_402436E8
                l32i.n          a2, a4, 0x14 ; g_ic.g.netif2;
                beqz.n          a2, loc_402436E4
                l32i.n          a2, a2, 0
                ret.n
; ---------------------------------------------------------------------------

loc_402436E4:                           ; CODE XREF: eagle_lwip_getif+12j
                movi.n          a2, 0
                ret.n
; ---------------------------------------------------------------------------

loc_402436E8:                           ; CODE XREF: eagle_lwip_getif:loc_402436D9j
                ret.n
; ---------------------------------------------------------------------------

loc_402436EA:                           ; CODE XREF: eagle_lwip_getif+7j
                l32i.n          a2, a2, 0
                ret.n
; End of function eagle_lwip_getif