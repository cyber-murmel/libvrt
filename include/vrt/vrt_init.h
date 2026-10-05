#ifndef INCLUDE_VRT_VRT_INIT_H_
#define INCLUDE_VRT_VRT_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

struct vrt_command;
struct vrt_fields;
struct vrt_header;
struct vrt_if_context;
struct vrt_packet;
struct vrt_trailer;

/**
 * Initialize header to a reasonable default state.
 *
 * \param header Header.
 *
 * \note Packet size must still be set to the actual packet size in 32-bit words.
 */
void vrt_init_header(struct vrt_header* header);

/**
 * Initialize fields section to a reasonable default state.
 *
 * \param fields Fields.
 */
void vrt_init_fields(struct vrt_fields* fields);

/**
 * Initialize trailer to a reasonable default state.
 *
 * \param trailer Trailer.
 */
void vrt_init_trailer(struct vrt_trailer* trailer);

/**
 * Initialize IF context to a reasonable default state.
 *
 * \param if_context IF context.
 */
void vrt_init_if_context(struct vrt_if_context* if_context);

/**
 * Initialize command section of command packets to a reasonable default state (VITA 49.2).
 *
 * \param command Command section.
 */
void vrt_init_command(struct vrt_command* command);

/**
 * Initialize VRT packet to a reasonable default state.
 *
 * \param packet VRT packet.
 *
 * \note Sets packet->command to NULL. Point it to a command section, e.g. initialized by vrt_init_command(), for
 *       command packets.
 */
void vrt_init_packet(struct vrt_packet* packet);

#ifdef __cplusplus
}
#endif

#endif
