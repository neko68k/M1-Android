
extern "C" {
extern unsigned char gunforce_decryption_table[256]; 
extern unsigned char bomberman_decryption_table[256]; 
extern unsigned char lethalth_decryption_table[256];  
extern unsigned char dynablaster_decryption_table[256]; 
extern unsigned char mysticri_decryption_table[256]; 
extern unsigned char majtitl2_decryption_table[256]; 
extern unsigned char hook_decryption_table[256]; 
extern unsigned char rtypeleo_decryption_table[256]; 
extern unsigned char inthunt_decryption_table[256]; 
extern unsigned char gussun_decryption_table[256]; 
extern unsigned char leagueman_decryption_table[256];
extern unsigned char psoldier_decryption_table[256]; 
extern unsigned char dsoccr94_decryption_table[256]; 
extern unsigned char shisen2_decryption_table[256]; 

extern unsigned char test_decryption_table[256];
}

extern void irem_cpu_decrypt(int cpu,const unsigned char *decryption_table);
