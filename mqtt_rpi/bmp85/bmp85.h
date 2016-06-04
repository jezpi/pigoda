#ifndef _BMP85_H_
#define  _BMP85_H_


struct bmp85 {
	double pressure;
	float temperature;
};
extern struct bmp85 *bmp85_getdata();
extern void bmp85_init(void);
#endif /* ! _BMP85_H_ */
