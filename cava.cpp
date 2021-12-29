#include <stdio.h>
#include <stdlib.h>
#include "cava.hpp"

void rewriteConfig(int sensitivity) {
   const char *top_config =
       "[general]\n"
       "bars = 30\n"
       "autosens = 0\n"
       "sensitivity = ";

   const char *bottom_config =
       "[output]\n"
       "method = raw\n"
       "raw_target = cava_fifo\n"
       "bit_format = 8bit\n"
       "channels = mono\n"
       "mono_option = left\n";

   FILE * fp;

   fp = fopen ("cava.conf", "w+");
   fprintf(fp, "%s%i\n\n%s", top_config, sensitivity, bottom_config);

   fclose(fp);
}

void reloadConfig() {
    system("pkill -USR1 cava");
}
