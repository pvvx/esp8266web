/*
https://github.com/freebsd/freebsd/blob/master/sys/net80211/ieee80211_proto.h
https://github.com/freebsd/freebsd/blob/master/sys/net80211/ieee80211_output.c
*/


struct ieee80211vap;


ieee80211_output_pbuf ??
ieee80211_tx_mgt_cb ??

void	ieee80211_send_setup(struct ieee80211_node *, struct mbuf *, int, int,
        const uint8_t [IEEE80211_ADDR_LEN], const uint8_t [IEEE80211_ADDR_LEN],
        const uint8_t [IEEE80211_ADDR_LEN]);
int	ieee80211_mgmt_output(struct ieee80211_node *, struct mbuf *, int,
		struct ieee80211_bpf_params *);
        
int	ieee80211_send_nulldata(struct ieee80211_node *);
        
uint8_t *ieee80211_add_rates(uint8_t *, const struct ieee80211_rateset *);
uint8_t *ieee80211_add_xrates(uint8_t *, const struct ieee80211_rateset *);

int	ieee80211_send_probereq(struct ieee80211_node *ni,
		const uint8_t sa[IEEE80211_ADDR_LEN],
		const uint8_t da[IEEE80211_ADDR_LEN],
		const uint8_t bssid[IEEE80211_ADDR_LEN],
		const uint8_t *ssid, size_t ssidlen);

uint16_t ieee80211_getcapinfo(struct ieee80211vap *,
		struct ieee80211_channel *);

int	ieee80211_send_mgmt(struct ieee80211_node *, int, int); 

struct mbuf *ieee80211_alloc_proberesp(struct ieee80211_node *, int);

int	ieee80211_send_proberesp(struct ieee80211vap *,
		const uint8_t da[IEEE80211_ADDR_LEN], int);

/*
 * Beacon frames constructed by ieee80211_beacon_alloc
 * have the following structure filled in so drivers
 * can update the frame later w/ minimal overhead.
 */
struct ieee80211_beacon_offsets {
	uint8_t		bo_flags[4];	/* update/state flags */
	uint16_t	*bo_caps;	/* capabilities */
	uint8_t		*bo_cfp;	/* start of CFParms element */
	uint8_t		*bo_tim;	/* start of atim/dtim */
	uint8_t		*bo_wme;	/* start of WME parameters */
	uint8_t		*bo_tdma;	/* start of TDMA parameters */
	uint8_t		*bo_tim_trailer;/* start of fixed-size trailer */
	uint16_t	bo_tim_len;	/* atim/dtim length in bytes */
	uint16_t	bo_tim_trailer_len;/* tim trailer length in bytes */
	uint8_t		*bo_erp;	/* start of ERP element */
	uint8_t		*bo_htinfo;	/* start of HT info element */
	uint8_t		*bo_ath;	/* start of ATH parameters */
	uint8_t		*bo_appie;	/* start of AppIE element */
	uint16_t	bo_appie_len;	/* AppIE length in bytes */
	uint16_t	bo_csa_trailer_len;
	uint8_t		*bo_csa;	/* start of CSA element */
	uint8_t		*bo_quiet;	/* start of Quiet element */
	uint8_t		*bo_meshconf;	/* start of MESHCONF element */
	uint8_t		*bo_spare[3];
};
struct mbuf *ieee80211_beacon_alloc(struct ieee80211_node *,
		struct ieee80211_beacon_offsets *);
