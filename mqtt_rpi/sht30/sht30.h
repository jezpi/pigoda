#ifndef _SHT30_H_
#define  _SHT30_H_


struct sht30 {
	double humidity;
	float temperature;
};
extern struct sht30 *sht30_getdata();
extern void sht30_init(void);
#endif /* ! _SHT30_H_ */
