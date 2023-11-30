#ifndef __TVW_H__
#define __TVW_H__
#include <stdio.h>
#include <stdint.h>

typedef struct _net {
  uint32_t _index;
  char *name;
} tvw_net;

typedef struct _pin {
  uint32_t pad_id;
  uint32_t unknown1;
  uint32_t pin_number;
  char     *name;
  uint32_t unknown2;
} tvw_pin;

typedef struct _part {
  uint32_t _index;
  char     *name;
  int32_t  corner1_x;
  int32_t  corner1_y;
  int32_t  corner2_x;
  int32_t  corner2_y;
  int32_t  pos_x;
  int32_t  pos_y;
  uint32_t rotation;
  uint32_t unknown1;
  uint32_t type;
  uint32_t unknown2;
  uint32_t unknown3;
  uint8_t  has_strings;
  char     *value;
  char     *string3;
  char     *string4;
  char     *package;
  char     *serial;
  uint32_t unknown4;
  uint32_t numpins;
  uint32_t layer; // e.g. 2 = TOP; 7 = BOTTOM - important to get the correct pad reference
  uint32_t unknown5;
  tvw_pin  *pins;
} tvw_part;

// pad extra data:
// so far I've only seen one type (Type 1) which is a total length of 16 bytes
typedef struct _pad_extdata {
  uint32_t unknown1_x;
  uint32_t unknown1_y;
  uint32_t unknown2_x;
  uint32_t unknown2_y;
} tvw_pad_extdata;

typedef struct _pad {
  uint32_t _index;
  uint32_t net_id;
  uint32_t dcode_id;
  int32_t  pos_x;
  int32_t  pos_y;
  uint8_t  flag1;
  uint8_t  has_dimensions;
  uint8_t  unknown_flags[2];
  int32_t  corner1_x;
  int32_t  corner1_y;
  int32_t  corner2_x;
  int32_t  corner2_y;
  uint8_t  has_extdata1;
  uint8_t  has_extdata2; // never seen this

  tvw_pad_extdata *extdata1;
} tvw_pad;

typedef struct _ctx {
  FILE *file;
  size_t nets_offset;
  size_t pads_offset[8];
  size_t parts_offset;
  uint32_t numnets;
  tvw_net *nets;
  uint32_t numpads[32];
  tvw_pad *pads[32];
  uint32_t numparts;
  tvw_part *parts;
} tvw_ctx;

typedef enum _layertypes {
  DOCUMENT = 0,
  TOP,
  BOTTOM,
  SIGNAL,
  PWRGND,
  MASK_TOP,
  MASK_BOTTOM,
  SILK_TOP,
  SILK_BOTTOM,
  PASTE_TOP,
  PASTE_BOTTOM,
  DRILL,
  ROUT
} tvw_layertype;

typedef enum _aperturetype {
  ROUND = 0,
  RECT,
  ROUNDRECT,
  OBLONG,
  UNKNOWN,
  CUSTOM
} tvw_aperturetype;

typedef struct _point {
  int32_t x;
  int32_t y;
} tvw_point;

typedef struct _aperture {
  uint32_t width;
  uint32_t height;
  tvw_aperturetype type;
  uint32_t unknown1_00;
  char *name; // only for type 5 (CUSTOM)
  int32_t corner1_x;
  int32_t corner1_y;
  int32_t corner2_x;
  int32_t corner2_y;
  uint32_t unknown2_01;
  uint32_t unknown3_02;
  uint32_t unknown4_01;
  uint32_t unknown5_00;
  uint32_t unknown6_00;
  uint32_t numvertices;
  tvw_point *vertices;
} tvw_aperture;

typedef struct _line {
  // record length: 0x18
  int32_t net_id;
  uint32_t aperture_id;
  tvw_point start;
  tvw_point end;
} tvw_line;

typedef struct _arc {
  // record length: 0x1c
  int32_t net_id;
  uint32_t aperture_id;
  tvw_point center;
  uint32_t radius;
  float start_degrees;
  float end_degrees;
} tvw_arc;

typedef struct _poly {

} tvw_poly;

typedef struct _label {
  char *text;
  int32_t pos_x;
  int32_t pos_y;
  uint32_t unknown;
  uint32_t rotation;
  uint8_t unknown1[15];
  uint32_t unknown2;
} tvw_label;

typedef struct _layer {
  uint32_t _index;
  uint32_t layerclass; //0 or 1; 0 is followed by a 3-2-1; 1 is followed by 2-1
  uint32_t preamble[3]; // only 2 words are used for class 1
  char *name1;
  char *name2;
  char *filename;
  tvw_layertype type;
  uint8_t color1[4];
  uint8_t color2[4];
  uint32_t numapertures;
  tvw_aperture *apertures;
  tvw_line *lines;
  tvw_arc *arcs;
  tvw_poly *polys;
  tvw_label *labels;
} tvw_layer;

void tvw_init(tvw_ctx *ctx, FILE *in, size_t nets_offset, size_t pads_offset_top, size_t pads_offset_bottom, size_t parts_offset);
void tvw_read_nets(tvw_ctx *ctx);
void tvw_read_pads(tvw_ctx *ctx, int layer);
void tvw_read_parts(tvw_ctx *ctx);

void tvw_dump_nets(tvw_ctx *ctx);
void tvw_dump_pads(tvw_ctx *ctx, int layer);
void tvw_dump_parts(tvw_ctx *ctx);

#endif