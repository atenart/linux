/*
 * Copyright (C) 2017, Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __AL_HW_UDMA_H__
#define __AL_HW_UDMA_H__

#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/types.h>

#include "al_udma_regs.h"
#include "al_hw_udma_regs.h"

#define DMA_MAX_Q 	4
#define AL_UDMA_MIN_Q_SIZE 	4
#define AL_UDMA_MAX_Q_SIZE 	BIT(16) /* hw can do more, but we limit it */

#define AL_UDMA_REV_ID_2	2

#define DMA_RING_ID_MASK	0x3
/* New registers ?? */
/* Statistics - TBD */

/* UDMA submission descriptor */
union al_udma_desc {
	/* TX */
	struct {
		u32 len_ctrl;
		u32 meta_ctrl;
		u64 buf_ptr;
	} tx;
	/* TX Meta, used by upper layer */
	struct {
		u32 len_ctrl;
		u32 meta_ctrl;
		u32 meta1;
		u32 meta2;
	} tx_meta;
	/* RX */
	struct {
		u32 len_ctrl;
		u32 buf2_ptr_lo;
		u64 buf1_ptr;
	} rx;
} __attribute__((aligned(16)));

/* TX desc length and control fields */

#define AL_M2S_DESC_CONCAT			BIT(31)
#define AL_M2S_DESC_NO_SNOOP_H			BIT(29)
#define AL_M2S_DESC_INT_EN			BIT(28)
#define AL_M2S_DESC_LAST			BIT(27)
#define AL_M2S_DESC_FIRST			BIT(26)
#define AL_M2S_DESC_RING_ID_SHIFT		24
#define AL_M2S_DESC_RING_ID_MASK		(0x3 << AL_M2S_DESC_RING_ID_SHIFT)
#define AL_M2S_DESC_META_DATA			BIT(23)
#define AL_M2S_DESC_LEN_SHIFT			0
#define AL_M2S_DESC_LEN_MASK			(0xfffff << AL_M2S_DESC_LEN_SHIFT)

#define AL_S2M_DESC_DUAL_BUF			BIT(31)
#define AL_S2M_DESC_RING_ID_SHIFT		24
#define AL_S2M_DESC_LEN_SHIFT			0
#define AL_S2M_DESC_LEN_MASK			(0xffff << AL_S2M_DESC_LEN_SHIFT)
#define AL_S2M_DESC_LEN2_SHIFT			16
#define AL_S2M_DESC_LEN2_MASK			(0x3fff << AL_S2M_DESC_LEN2_SHIFT)
#define AL_S2M_DESC_LEN2_GRANULARITY_SHIFT	6

/* TX/RX descriptor Target-ID field (in the buffer address 64 bit field) */
#define AL_UDMA_DESC_TGTID_SHIFT		48

/* UDMA completion descriptor */
union al_udma_cdesc {
	/* TX completion */
	struct {
		u32 ctrl_meta;
	} al_desc_comp_tx;
	/* RX completion */
	struct {
		u32 ctrl_meta;
	} al_desc_comp_rx;
} __attribute__((aligned(4)));

/* TX/RX common completion desc ctrl_meta feilds */
#define AL_UDMA_CDESC_ERROR		BIT(31)
#define AL_UDMA_CDESC_LAST		BIT(27)
#define AL_UDMA_CDESC_BUF2_USED		BIT(31)

/* Basic Buffer structure */
struct al_buf {
	/* Buffer physical address */
	dma_addr_t addr;
	/* Buffer lenght in bytes */
	u32 len;
};

/* UDMA type */
enum al_udma_type {
	UDMA_TX,
	UDMA_RX
};

/* UDMA state */
enum al_udma_state {
	UDMA_DISABLE = 0,
	UDMA_IDLE,
	UDMA_NORMAL,
	UDMA_ABORT,
	UDMA_RESET
};

extern const char *const al_udma_states_name[];

/* UDMA Q specific parameters from upper layer */
struct al_udma_q_params {
	/*
	 * ring size (in descriptors), submission and completion rings must have
	 * the same size
	 */
	u32 size;
	/* cpu address for submission ring descriptors */
	union al_udma_desc *desc_base;
	/* submission ring descriptors physical base address */
	dma_addr_t desc_phy_base;
	 /* completion descriptors pointer, NULL means no completion update */
	u8 *cdesc_base;
	/* completion descriptors ring physical base address */
	dma_addr_t cdesc_phy_base;

	u8 adapter_rev_id;
};

/* UDMA parameters from upper layer */
struct al_udma_params {
	struct device *dev;
	void __iomem *udma_regs_base;
	enum al_udma_type type;
	/* size (in bytes) of the udma completion ring descriptor */
	u32 cdesc_size;
	u8 num_of_queues;
	const char *name;
};

/* Fordward decleration */
struct al_udma;

/* SW status of a queue */
enum al_udma_queue_status {
	AL_QUEUE_NOT_INITIALIZED = 0,
	AL_QUEUE_DISABLED,
	AL_QUEUE_ENABLED,
	AL_QUEUE_ABORTED
};

/* UDMA Queue private data structure */
struct al_udma_q {
	/* mask used for pointers wrap around equals to size - 1 */
	u16 size_mask;
	/* pointer to the per queue UDMA registers */
	void __iomem *q_regs;
	/* base address submission ring descriptors */
	union al_udma_desc *desc_base_ptr;
	/* index to the next available submission descriptor */
	u16 next_desc_idx;
	/* current submission ring id */
	u32 desc_ring_id;
	/* completion descriptors pointer, NULL means no completion */
	u8 *cdesc_base_ptr;
	/* index in descriptors for next completing ring descriptor */
	u16 next_cdesc_idx;
	/* used for wrap around detection */
	u8 *end_cdesc_ptr;
	/* completion ring head pointer register shadow */
	u16 comp_head_idx;
	/*
	 * when working in get_packet mode we maintain pointer instead of the
	 * above id
	 */
	volatile union al_udma_cdesc *comp_head_ptr;

	/* holds the number of processed descriptors of the current packet */
	u32 pkt_crnt_descs;
	/* current completion Ring Id */
	u32 comp_ring_id;

	dma_addr_t desc_phy_base;	/* submission desc. physical base */
	dma_addr_t cdesc_phy_base;	/* completion desc. physical base */

	u32 flags;			/* flags used for completion modes */
	u32 size;			/* ring size in descriptors  */
	enum al_udma_queue_status status;
	struct al_udma *udma;		/* pointer to parent UDMA */
	u32 qid;			/* the index number of the queue */

	/*
	 * The following fields are duplicated from the UDMA parent adapter
	 * due to performance considerations.
	 */
	u8 adapter_rev_id;
} ____cacheline_aligned;

/* UDMA */
struct al_udma {
	const char *name;
	struct device *dev;
	enum al_udma_type type;		/* Tx or Rx */
	enum al_udma_state state;
	/* size (in bytes) of the udma completion ring descriptor */
	u32 cdesc_size;
	u8 num_of_queues;
	void __iomem *udma_regs;
	struct udma_gen_regs *gen_regs;
	struct al_udma_q udma_q[DMA_MAX_Q];
	unsigned int rev_id;
};

/*
 * Initialize the udma engine
 *
 * @param udma udma data structure
 * @param udma_params udma parameters from upper layer
 *
 * @return 0 on success. -EINVAL otherwise.
 */
int al_udma_init(struct al_udma *udma, struct al_udma_params *udma_params);

/*
 * Initialize the udma queue data structure
 *
 * @param udma
 * @param qid
 * @param q_params
 *
 * @return 0 if no error found.
 *	   -EINVAL if the qid is out of range
 *	   -EIO if queue was already initialized
 */

int al_udma_q_init(struct al_udma *udma, u32 qid,
		   struct al_udma_q_params *q_params);

/*
 * return (by reference) a pointer to a specific queue date structure.
 * this pointer needed for calling functions (i.e. al_udma_desc_action_add) that
 * require this pointer as input argument.
 *
 * @param udma udma data structure
 * @param qid queue index
 * @param q_handle pointer to the location where the queue structure pointer
 * written to.
 *
 * @return  0 on success. -EINVAL otherwise.
 */
int al_udma_q_handle_get(struct al_udma *udma, u32 qid,
			 struct al_udma_q **q_handle);

/*
 * Change the UDMA's state
 *
 * @param udma udma data structure
 * @param state the target state
 */
void al_udma_state_set(struct al_udma *udma, enum al_udma_state state);

/*
 * return the current UDMA hardware state
 *
 * @param udma udma handle
 *
 * @return the UDMA state as reported by the hardware.
 */
enum al_udma_state al_udma_state_get(struct al_udma *udma);

/*
 * Action handling
 */

/*
 * get number of descriptors that can be submitted to the udma.
 * keep one free descriptor to simplify full/empty management
 * @param udma_q queue handle
 *
 * @return num of free descriptors.
 */
static inline u32 al_udma_available_get(struct al_udma_q *udma_q)
{
	u16 tmp = udma_q->next_cdesc_idx - (udma_q->next_desc_idx + 1);
	tmp &= udma_q->size_mask;

	return (u32) tmp;
}

/*
 * get next available descriptor
 * @param udma_q queue handle
 *
 * @return pointer to the next available descriptor
 */
static inline union al_udma_desc *al_udma_desc_get(struct al_udma_q *udma_q)
{
	union al_udma_desc *desc;
	u16 next_desc_idx;

	next_desc_idx = udma_q->next_desc_idx;
	desc = udma_q->desc_base_ptr + next_desc_idx;

	next_desc_idx++;

	/* if reached end of queue, wrap around */
	udma_q->next_desc_idx = next_desc_idx & udma_q->size_mask;

	return desc;
}

/*
 * get ring id for the last allocated descriptor
 * @param udma_q
 *
 * @return ring id for the last allocated descriptor
 * this function must be called each time a new descriptor is allocated
 * by the al_udma_desc_get(), unless ring id is ignored.
 */
static inline u32 al_udma_ring_id_get(struct al_udma_q *udma_q)
{
	u32 ring_id;

	ring_id = udma_q->desc_ring_id;

	/* calculate the ring id of the next desc */
	/* if next_desc points to first desc, then queue wrapped around */
	if (unlikely(udma_q->next_desc_idx) == 0)
		udma_q->desc_ring_id = (udma_q->desc_ring_id + 1) &
			DMA_RING_ID_MASK;
	return ring_id;
}

/* add DMA action - trigger the engine */
/*
 * add num descriptors to the submission queue.
 *
 * @param udma_q queue handle
 * @param num number of descriptors to add to the queues ring.
 */
static inline void al_udma_desc_action_add(struct al_udma_q *udma_q, u32 num)
{
	u32 *addr;

	addr = udma_q->q_regs + UDMA_RINGS_DRTP_INC;
	/*
	 * make sure data written to the descriptors will be visible by the
	 * DMA
	 */
	wmb();

	writel_relaxed(num, addr);
}

#define cdesc_is_last(flags) ((flags) & AL_UDMA_CDESC_LAST)

/*
 * return pointer to the cdesc + offset desciptors. wrap around when needed.
 *
 * @param udma_q queue handle
 * @param cdesc pointer that set by this function
 * @param offset offset desciptors
 *
 */
static inline volatile union al_udma_cdesc *al_cdesc_next(
					struct al_udma_q *udma_q,
					volatile union al_udma_cdesc *cdesc,
					u32 offset)
{
	volatile u8 *tmp = (volatile u8 *) cdesc + offset * udma_q->udma->cdesc_size;

	/* if wrap around */
	if (unlikely((tmp > udma_q->end_cdesc_ptr)))
		return (union al_udma_cdesc *)
			(udma_q->cdesc_base_ptr +
			(tmp - udma_q->end_cdesc_ptr - udma_q->udma->cdesc_size));

	return (volatile union al_udma_cdesc *) tmp;
}

/*
 * check if the flags of the descriptor indicates that is new one
 * the function uses the ring id from the descriptor flags to know whether it
 * new one by comparing it with the curring ring id of the queue
 *
 * @param udma_q queue handle
 * @param flags the flags of the completion descriptor
 *
 * @return true if the completion descriptor is new one.
 * 	false if it old one.
 */
static inline bool al_udma_new_cdesc(struct al_udma_q *udma_q, u32 flags)
{
	if (((flags & AL_M2S_DESC_RING_ID_MASK) >> AL_M2S_DESC_RING_ID_SHIFT)
	    == udma_q->comp_ring_id)
		return true;
	return false;
}

/*
 * get next completion descriptor
 * this function will also increment the completion ring id when the ring wraps
 * around
 *
 * @param udma_q queue handle
 * @param cdesc current completion descriptor
 *
 * @return pointer to the completion descriptor that follows the one pointed by
 * cdesc
 */
static inline volatile union al_udma_cdesc *al_cdesc_next_update(
					struct al_udma_q *udma_q,
					volatile union al_udma_cdesc *cdesc)
{
	/* if last desc, wrap around */
	if (unlikely(((volatile u8 *) cdesc == udma_q->end_cdesc_ptr))) {
		udma_q->comp_ring_id =
		    (udma_q->comp_ring_id + 1) & DMA_RING_ID_MASK;
		return (union al_udma_cdesc *) udma_q->cdesc_base_ptr;
	}
	return (volatile union al_udma_cdesc *) ((volatile u8 *) cdesc + udma_q->udma->cdesc_size);
}

/*
 * get next completed packet from completion ring of the queue
 *
 * @param udma_q udma queue handle
 * @param desc pointer that set by this function to the first descriptor
 * note: desc is valid only when return value is not zero
 * @return number of descriptors that belong to the packet. 0 means no completed
 * full packet was found.
 * If the descriptors found in the completion queue don't form full packet (no
 * desc with LAST flag), then this function will do the following:
 * (1) save the number of processed descriptors.
 * (2) save last processed descriptor, so next time it called, it will resume
 *     from there.
 * (3) return 0.
 * note: the descriptors that belong to the completed packet will still be
 * considered as used, that means the upper layer is safe to access those
 * descriptors when this function returns. the al_udma_cdesc_ack() should be
 * called to inform the udma driver that those descriptors are freed.
 */
u32 al_udma_cdesc_packet_get(struct al_udma_q *udma_q,
			     volatile union al_udma_cdesc **desc);

/* get completion descriptor pointer from its index */
#define al_udma_cdesc_idx_to_ptr(udma_q, idx)				\
	((volatile union al_udma_cdesc *) ((udma_q)->cdesc_base_ptr +	\
				(idx) * (udma_q)->udma->cdesc_size))


/*
 * return number of all completed descriptors in the completion ring
 *
 * @param udma_q udma queue handle
 * @param cdesc pointer that set by this function to the first descriptor
 * note: desc is valid only when return value is not zero
 * note: pass NULL if not interested
 * @return number of descriptors. 0 means no completed descriptors were found.
 * note: the descriptors that belong to the completed packet will still be
 * considered as used, that means the upper layer is safe to access those
 * descriptors when this function returns. the al_udma_cdesc_ack() should be
 * called to inform the udma driver that those descriptors are freed.
 */
static inline u32 al_udma_cdesc_get_all(struct al_udma_q *udma_q,
					volatile union al_udma_cdesc **cdesc)
{
	u16 count = 0;

	udma_q->comp_head_idx = readl(udma_q->q_regs + UDMA_RINGS_CRHP) & 0xffff;
	count = (udma_q->comp_head_idx - udma_q->next_cdesc_idx) &
		udma_q->size_mask;

	if (cdesc)
		*cdesc = al_udma_cdesc_idx_to_ptr(udma_q, udma_q->next_cdesc_idx);

	return count;
}

/*
 * acknowledge the driver that the upper layer completed processing completion
 * descriptors
 *
 * @param udma_q udma queue handle
 * @param num number of descriptors to acknowledge
 */
static inline void al_udma_cdesc_ack(struct al_udma_q *udma_q, u32 num)
{
	udma_q->next_cdesc_idx += num;
	udma_q->next_cdesc_idx &= udma_q->size_mask;
}

#endif /* __AL_HW_UDMA_H__ */
