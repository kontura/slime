#include <stdio.h>
#include <stdlib.h>
#include "cava.hpp"

void rewriteConfig(int sensitivity, const char *read_from) {
   const char *general_config =
       "[general]\n"
       "bars = 30\n"
       "autosens = 0\n"
       "sensitivity = ";

   const char *output_config =
       "[output]\n"
       "method = raw\n"
       "raw_target = cava_fifo\n"
       "bit_format = 8bit\n"
       "data_format = binary\n"
       "channels = mono\n"
       "mono_option = average\n";

   FILE * fp;
   fp = fopen("cava.conf", "w+");

   if (read_from ) {
       const char *input_config_p1 =
           "[input]\n"
           "method = fifo\n"
           "source = ";
       const char * input_config_p2 =
           "sample_rate = 44100\n"
           "sample_bits = 16\n";

       fprintf(fp, "%s%i\n\n%s\n\n%s%s\n%s", general_config, sensitivity, output_config, input_config_p1, read_from, input_config_p2);
   } else {
       fprintf(fp, "%s%i\n\n%s\n", general_config, sensitivity, output_config);
   }

   fclose(fp);
}

void reloadConfig() {
    system("pkill -USR1 cava");
}

void runCava() {
    system("cava -p cava.conf &");
}

void stopCava() {
    system("pkill cava");
}
