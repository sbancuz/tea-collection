#include "cr.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct cr0 {
  unsigned int PE : 1; // 0
  unsigned int MP : 1; // 1
  unsigned int EM : 1; // 2
  unsigned int TS : 1; // 3
  unsigned int ET : 1; // 4
  unsigned int NE : 1; // 5
  unsigned int pad1 : 11;
  unsigned int WP : 1; // 16
  unsigned int pad2 : 1;
  unsigned int AM : 1; // 18
  unsigned int pad3 : 11;
  unsigned int NW : 1; // 29
  unsigned int CD : 1; // 30
  unsigned int PG : 1; // 31
};

struct cr4 {
  unsigned int VME : 1;
  unsigned int PVI : 1;
  unsigned int TSD : 1;
  unsigned int DE : 1;
  unsigned int PSE : 1;
  unsigned int PAE : 1;
  unsigned int MCE : 1;
  unsigned int PGE : 1;
  unsigned int PCE : 1;
  unsigned int OSFXSR : 1;
  unsigned int OSXMMEXCPT : 1;
  unsigned int UMIP : 1;
  unsigned int LA57 : 1;
  unsigned int VMXE : 1;
  unsigned int SMXE : 1;
  unsigned int FSGSBASE : 1;
  unsigned int PCIDE : 1;
  unsigned int OSXSAVE : 1;
  unsigned int SMEP : 1;
  unsigned int SMAP : 1;
  unsigned int PKE : 1;
  unsigned int CET : 1;
  unsigned int PKS : 1;
};

void print_cr0(struct cr0 *cr) {
  printf("CR0: 0x%08x\n", *(unsigned int *)cr);
  printf("PE: %d\n", cr->PE);
  printf("MP: %d\n", cr->MP);
  printf("EM: %d\n", cr->EM);
  printf("TS: %d\n", cr->TS);
  printf("ET: %d\n", cr->ET);
  printf("NE: %d\n", cr->NE);
  printf("WP: %d\n", cr->WP);
  printf("AM: %d\n", cr->AM);
  printf("NW: %d\n", cr->NW);
  printf("CD: %d\n", cr->CD);
  printf("PG: %d\n", cr->PG);
  printf("Reserved bits: %d\n", cr->pad1 + cr->pad2 + cr->pad3);
}

void print_cr4(struct cr4 *cr) {
  printf("CR4: 0x%08x\n", *(unsigned int *)cr);
  printf("Virtual 8086 Mode Extensions: %d\n", cr->VME);
  printf("Protected-mode Virtual Interrupts: %d\n", cr->PVI);
  printf("Time Stamp Disable: %d\n", cr->TSD);
  printf("Debugging Extensions: %d\n", cr->DE);
  printf("Page Size Extension: %d\n", cr->PSE);
  printf("Physical Address Extension: %d\n", cr->PAE);
  printf("Machine Check Exception: %d\n", cr->MCE);
  printf("Page Global Enable: %d\n", cr->PGE);
  printf("Performance-Monitoring Counter enable: %d\n", cr->PCE);

  printf("Operating system support for FXSAVE and FXRSTOR instructions: %d\n",
         cr->OSFXSR);
  printf("Operating System Support for Unmasked SIMD Floating-Point "
         "Exceptions: %d\n",
         cr->OSXMMEXCPT);
  printf("User-Mode Instruction Prevention: %d\n", cr->UMIP);
  printf("57-bit linear addresses: %d\n", cr->LA57);
  printf("Virtual Machine Extensions Enable: %d\n", cr->VMXE);
  printf("Safer Mode Extensions Enable: %d\n", cr->SMXE);
  printf("Enables the instructions RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE: "
         "%d\n",
         cr->FSGSBASE);
  printf("PCID Enable: %d\n", cr->PCIDE);
  printf("XSAVE and Processor Extended States Enable: %d\n", cr->OSXSAVE);
  printf("Supervisor Mode Execution Protection Enable: %d\n", cr->SMEP);
  printf("Supervisor Mode Access Prevention Enable: %d\n", cr->SMAP);
  printf("Protection Key Enable: %d\n", cr->PKE);
  printf("Control-flow Enforcement Technology: %d\n", cr->CET);
  printf("Enable Protection Keys for Supervisor-Mode Pages: %d\n", cr->PKS);
}

int main() {
  int fd;
  crs values = {0};

  fd = open("/dev/cr_device", O_RDWR);
  if (fd < 0) {
    printf("Failed to open device");
    return -1;
  }

  if (ioctl(fd, CR_READ, &values) < 0) {
    perror("Failed to read CR registers");
    close(fd);
    return -1;
  }

  print_cr0((struct cr0 *)&values.cr0);
  printf("CR2: %llx\n", values.cr2);
  print_cr4((struct cr4 *)&values.cr4);
  printf("CR4: %d\n", (unsigned char)values.cr4);

  close(fd);
  return 0;
}
