#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "tvw.h"

uint32_t read_u32le_from_file(FILE *in) {
  uint32_t val;
  val  = fgetc(in);
  val |= fgetc(in) << 8;
  val |= fgetc(in) << 16;
  val |= fgetc(in) << 24;
  return val;
}

int32_t read_s32le_from_file(FILE *in) {
  return (int32_t)read_u32le_from_file(in);
}

char *read_pascal_string_from_file(FILE *in) {
  char *retval;
  uint8_t len;
  fread(&len, 1, 1, in);
  retval = calloc(len + 1, 1); // add room for zero termination
  fread(retval, 1, len, in);
  retval[len] = 0;
  return retval;
}

void tvw_parse_header(tvw_ctx *ctx) {
  FILE *in = ctx->file;
  fseek(in, 0, SEEK_SET);
  read_pascal_string_from_file(in);
}

void tvw_parse_layers(tvw_ctx *ctx) {
  tvw_read_pads(ctx, TOP);
  tvw_read_pads(ctx, BOTTOM);
}

void tvw_parse_probedb(tvw_ctx *ctx) {

}

void tvw_init(tvw_ctx *ctx, FILE *in, size_t nets_offset, size_t pads_offset_top, size_t pads_offset_bottom, size_t parts_offset) {
  ctx->file = in;
  ctx->nets_offset = nets_offset;
  ctx->pads_offset[TOP] = pads_offset_top;
  ctx->pads_offset[BOTTOM] = pads_offset_bottom;
  ctx->parts_offset = parts_offset;
  tvw_parse_header(ctx);
  tvw_parse_layers(ctx);
  tvw_read_nets(ctx);
  tvw_parse_probedb(ctx);
  tvw_read_parts(ctx);
}


void tvw_read_pad(tvw_ctx *ctx, int layer, uint32_t index) {
  FILE *in = ctx->file;

  tvw_pad *tgt = &ctx->pads[layer][index];
  if(tgt == NULL) {
    printf("ERROR: target pad struct is NULL\n");
    return;
  }
  tgt->_index = index;
  fread(&tgt->net_id, 4, 1, in);
  fread(&tgt->dcode_id, 4, 1, in);
  fread(&tgt->pos_x, 4, 1, in);
  fread(&tgt->pos_y, 4, 1, in);
  fread(&tgt->flag1, 1, 1, in);
  fread(&tgt->has_dimensions, 1, 1, in);

  tgt->unknown_flags[0] = 0;
  tgt->unknown_flags[1] = 0;

  /* read more data when has_dimensions flag is non-zero. */
  if(tgt->has_dimensions) {
    fread(tgt->unknown_flags, 1, 2, in);

    /* dimensions */
    fread(&tgt->corner1_x, 4, 1, in);
    fread(&tgt->corner1_y, 4, 1, in);
    fread(&tgt->corner2_x, 4, 1, in);
    fread(&tgt->corner2_y, 4, 1, in);

    /* yet more extra data */
    fread(&tgt->has_extdata1, 1, 1, in);
    if(tgt->has_extdata1) {
      tvw_pad_extdata *extdata1 = calloc(1, sizeof(tvw_pad_extdata));
      tgt->extdata1 = extdata1;

      fread(&extdata1->unknown1_x, 4, 1, in);
      fread(&extdata1->unknown1_y, 4, 1, in);
      fread(&extdata1->unknown2_x, 4, 1, in);
      fread(&extdata1->unknown2_y, 4, 1, in);
    }
  }

  /* NOTE I never saw this value be non-zero. Extra data might have to be
     read when it is. */
  fread(&tgt->has_extdata2, 1, 1, in);
}

void tvw_read_pin(tvw_ctx *ctx, tvw_part *part, uint32_t index) {
  FILE *in = ctx->file;
  tvw_pin *tgt = &part->pins[index];

  if(tgt == NULL) {
    printf("ERROR: target pin struct is NULL\n");
    return;
  }

  fread(&tgt->pad_id, 4, 1, in);
  tgt->pad_id >>= 3;
  fread(&tgt->unknown1, 4, 1, in);
  fread(&tgt->pin_number, 4, 1, in);
  tgt->name = read_pascal_string_from_file(in);
  fread(&tgt->unknown2, 4, 1, in);
}

void tvw_read_part(tvw_ctx *ctx, uint32_t index) {
  FILE *in = ctx->file;

  tvw_part *tgt = &ctx->parts[index];
  if(tgt == NULL) {
    printf("ERROR: target part struct is NULL\n");
    return;
  }
  tgt->_index = index;
  tgt->name = read_pascal_string_from_file(in);
  fread(&tgt->corner1_x, 4, 1, in);
  fread(&tgt->corner1_y, 4, 1, in);
  fread(&tgt->corner2_x, 4, 1, in);
  fread(&tgt->corner2_y, 4, 1, in);
  fread(&tgt->pos_x, 4, 1, in);
  fread(&tgt->pos_y, 4, 1, in);
  fread(&tgt->rotation, 4, 1, in);
  fread(&tgt->unknown1, 4, 1, in);
  fread(&tgt->type, 4, 1, in);
  fread(&tgt->unknown2, 4, 1, in);
  fread(&tgt->unknown3, 4, 1, in);
  fread(&tgt->has_strings, 1, 1, in);
  if(tgt->has_strings) {
    tgt->value = read_pascal_string_from_file(in);
    tgt->string3 = read_pascal_string_from_file(in);
    tgt->string4 = read_pascal_string_from_file(in);
    tgt->package = read_pascal_string_from_file(in);
    tgt->serial = read_pascal_string_from_file(in);
  }
  fread(&tgt->unknown4, 4, 1, in);
  fread(&tgt->numpins, 4, 1, in);
  fread(&tgt->layer, 4, 1, in);
  fread(&tgt->unknown5, 4, 1, in);
  tgt->pins = calloc(tgt->numpins, sizeof(tvw_pin));
  for(int i = 0; i < tgt->numpins; i++) {
    tvw_read_pin(ctx, tgt, i);
  }
}


void tvw_read_nets(tvw_ctx *ctx) {
  FILE *in = ctx->file;
  fseek(in, ctx->nets_offset, SEEK_SET);

  uint32_t numnets;
  fread(&numnets, 4, 1, in);
  printf("Total number of nets: %d\n", numnets);
  fseek(in, 4, SEEK_CUR); // skip second copy of net count

  ctx->numnets = numnets;
  ctx->nets = calloc(numnets, sizeof(tvw_net));

  for(int i = 0; i < numnets; i++) {
    char *name = read_pascal_string_from_file(in);
    ctx->nets[i]._index = i;
    ctx->nets[i].name = name;
  }
}

void tvw_read_pads(tvw_ctx *ctx, int layer) {
  FILE *in = ctx->file;

  fseek(in, ctx->pads_offset[layer], SEEK_SET);
  fread(&ctx->numpads[layer], 4, 1, in);
  printf("Total number of pads on layer %d: %d\n", layer, ctx->numpads[layer]);
  fseek(in, 4, SEEK_CUR); // skip unknown 0x00000002 field
  ctx->pads[layer] = calloc(ctx->numpads[layer], sizeof(tvw_pad));

  for(int i = 0; i < ctx->numpads[layer]; i++) {
    tvw_read_pad(ctx, layer, i);
  }
}

void tvw_read_parts(tvw_ctx *ctx) {
  FILE *in = ctx->file;

  fseek(in, ctx->parts_offset, SEEK_SET);
  fread(&ctx->numparts, 4, 1, in);
  printf("Total number of parts: %d\n", ctx->numparts);
  fseek(in, 4, SEEK_CUR); // skip second count field
  ctx->parts = calloc(ctx->numparts, sizeof(tvw_part));

  for(int i = 0; i < ctx->numparts; i++) {
    tvw_read_part(ctx, i);
  }
}


void print_pad(tvw_ctx *ctx, const tvw_pad pad){
  printf("PAD no.%d: NET=%d (%s) DCODE=%d X=%d Y=%d flag1=%d has_dimensions=%d flags(2,3)=(%d,%d)\n",
    pad._index, pad.net_id, ctx->nets[pad.net_id].name, pad.dcode_id,
    pad.pos_x, pad.pos_y,
    pad.flag1, pad.has_dimensions,
    pad.unknown_flags[0], pad.unknown_flags[1]);
}

void print_part(tvw_ctx *ctx, const tvw_part part) {
  printf("\nPART no.%d: name=%s value=%s pkg=%s layer=%d pins=%d\n",
    part._index, part.name, part.value, part.package, part.layer, part.numpins);
  printf("--> PINS:\n");
  for(int i = 0; i < part.numpins; i++) {
    tvw_pin pin = part.pins[i];
    uint32_t pad_id = pin.pad_id;

    uint32_t layer = part.layer;
    tvw_pad pad = ctx->pads[layer][pad_id];

    uint32_t net_id = pad.net_id;
    tvw_net net = ctx->nets[net_id];

    printf("    PIN no.%d: name=%s pad=%d (layer %d (%d,%d)) --> net=%d (%s)\n",
      i, pin.name, pin.pad_id, layer, pad.pos_x, pad.pos_y, net_id, net.name);
  }
}

void tvw_dump_nets(tvw_ctx *ctx) {
  printf("\n\nNETS\n====\n");
  for(int i = 0; i < ctx->numnets; i++) {
    printf("NET no.%d: %s\n", i, ctx->nets[i].name);
  }
}

void tvw_dump_pads(tvw_ctx *ctx, int layer) {
  printf("\n\nPADS ON LAYER %d:\n================\n", layer);
  for(int i = 0; i < ctx->numpads[layer]; i++) {
    print_pad(ctx, ctx->pads[layer][i]);
  }
}

void tvw_dump_parts(tvw_ctx *ctx) {
  printf("\n\nPARTS:\n======\n");
  for(int i = 0; i < ctx->numparts; i++) {
    print_part(ctx, ctx->parts[i]);
  }
}
