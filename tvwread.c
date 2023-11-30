#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "tvw.h"

typedef struct cfg {
  bool read_parts;
  bool read_pads;
  uint32_t parts_offset;
  uint32_t pads_offset_top;
  uint32_t pads_offset_bottom;
  uint32_t nets_offset;
  char *infile;
} cfg_t;

cfg_t cfg = {
  .read_parts = false,
  .read_pads = false,
  .parts_offset = 0,
  .pads_offset_top = 0,
  .pads_offset_bottom = 0,
  .nets_offset = 0,
  .infile = NULL
};

void print_usage(const char *prog) {
  printf("\nUsage: %s -n <offset> -p <offset> -t <offset> -b <offset> <input file>\n", prog);
  printf("  input file      TVW file to be read\n");
  printf("  -n --nets <offset>    offset of nets table.\n");
  printf("  -p --parts <offset>   offset of parts table.\n");
  printf("  -t --top <offset>     offset of pads table (top layer).\n");
  printf("  -b --bottom <offset>  offset of pads table (bottom layer).\n");
  printf("Output is logged to stdout.\n");
}

int parse_opts(int argc, char **argv) {
  static const struct option lopts[] = {
    { "nets",   required_argument, NULL, 'n' },
    { "parts",  required_argument, NULL, 'p' },
    { "top",    required_argument, NULL, 't' },
    { "bottom", required_argument, NULL, 'b' },
    { NULL, no_argument, NULL, 0 }
  };

  typedef enum opts {
    OPT_INFILE        = 0x01,
    OPT_NETS_OFFSET   = 0x02,
    OPT_PARTS_OFFSET  = 0x04,
    OPT_TOP_OFFSET    = 0x08,
    OPT_BOTTOM_OFFSET = 0x10
  } opts;

  opts missing_opts = OPT_INFILE | OPT_NETS_OFFSET | OPT_PARTS_OFFSET
                    | OPT_TOP_OFFSET | OPT_BOTTOM_OFFSET;

  while(true) {
    int c;

    c = getopt_long(argc, argv, "n:p:t:b:", lopts, NULL);

    if(c == -1) {
      break;
    }

    switch(c) {
      case 'p':
        cfg.parts_offset = strtol(optarg, NULL, 0);
        cfg.read_parts = true;
        missing_opts &= ~OPT_PARTS_OFFSET;
        break;

      case 'n':
        cfg.nets_offset = strtol(optarg, NULL, 0);
        missing_opts &= ~OPT_NETS_OFFSET;
        break;

      case 't':
        cfg.pads_offset_top = strtol(optarg, NULL, 0);
        cfg.read_pads = true;
        missing_opts &= ~OPT_TOP_OFFSET;
        break;

      case 'b':
        cfg.pads_offset_bottom = strtol(optarg, NULL, 0);
        cfg.read_pads = true;
        missing_opts &= ~OPT_BOTTOM_OFFSET;
        break;

      case '?':
        return 1;
        break;
    }
  }

  for(int index = optind; index < argc; index++) {
    if(cfg.infile != NULL) {
      fprintf(stderr, "Extra argument `%s'.\n", argv[index]);
      missing_opts |= OPT_INFILE;
    } else {
      missing_opts &= ~OPT_INFILE;
      cfg.infile = argv[index];
    }
  }

  return missing_opts;
}

int main(int argc, char **argv) {
  FILE *in;

  if(parse_opts(argc, argv)) {
    print_usage(argv[0]);
    abort();
  }

  printf("Reading from input file %s\n", cfg.infile);

  if((in=fopen(cfg.infile, "rb+"))==NULL) {
    perror("Can't open input file");
    abort();
  }

  tvw_ctx ctx;
  tvw_init(&ctx, in, cfg.nets_offset, cfg.pads_offset_top, cfg.pads_offset_bottom, cfg.parts_offset);

  tvw_dump_nets(&ctx);
  tvw_dump_pads(&ctx, TOP);
  tvw_dump_pads(&ctx, BOTTOM);
  tvw_dump_parts(&ctx);

  printf("Done.\n");
  fclose(in);

  return 0;
}