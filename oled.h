void oled_setup(void);
void oled_data_mode(void);
void oled_cmd_mode(void);

void Set_ColumnAddress(uint8_t, uint8_t);
void Set_PageAddress(uint8_t, uint8_t);
void oled_write_char(uint8_t c);

void Set_Page(uint8_t);
void Set_Column(uint8_t);
