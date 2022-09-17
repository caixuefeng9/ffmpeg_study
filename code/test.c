
#define MAX_AL_DATA_LEN         60
#define FILTER_DATA_LEN         10  // MAX+MIN, the actual len is double

typedef struct {
    uint32_t magic;
    int16_t A0_val;
    int16_t max_val;
    int16_t min_val;
    uint16_t cur_pos;
    uint16_t data_num;
    int16_t AL_data[MAX_AL_DATA_LEN];
} DF100_AL_DATA;

static DF100_AL_DATA df100_al;

int16_t max_val_array[FILTER_DATA_LEN] = {0x7FFF};
int16_t min_val_array[FILTER_DATA_LEN] = {0xFFFF};

static int binary_search(unsigned int vals[], int size, unsigned int key)
{
	int left  = 0;
	int right = size - 1;
	int mid	  = (left + right) / 2;

	if (key <= vals[left])
		return left;

	if (key >= vals[right])
		return right;

	while (left <= right) {
		mid = (left + right) / 2;

		if (key == vals[mid])
			return mid;

		if (key > vals[mid])
			left = mid + 1;
		else
			right = mid - 1;
	}

	return mid;
}

void df100_update_max_min_array(int16_t al_val)
{
    uint16_t pos;

    df100_al.max_val = max_val_array[0];
    df100_al.min_val = min_val_array[0];

    if (al_val > df100_al.max_val) {
        pos = binary_search(max_val_array, FILTER_DATA_LEN, al_val);
        max_val_array[pos] = al_val;
    } else if (al_val < df100_al.min_val) {
        pos = binary_search(min_val_array, FILTER_DATA_LEN, al_val);
        min_val_array[pos] = al_val;
    }
}

void df100_al_data_write(int16_t al_val)
{
    if (df100_al.data_num <= MAX_AL_DATA_LEN) {
        df100_al.cur_pos = df100_al.data_num - 1;
        df100_al.AL_data [df100_al.cur_pos] = al_val;
        df100_al.data_num++;
    } else {
        if (df100_al.cur_pos == MAX_AL_DATA_LEN - 1)
            df100_al.cur_pos = 0;

        df100_al.AL_data [df100_al.cur_pos] = al_val;
        df100_al.cur_pos++;
    }

    df100_update_max_min_array(al_val);
}

int rand_data[100];  

int main(void)
{
	int num = 0;
	while (num < 100) {
		df100_al_data_write(rand_data[i]);
		num++
	}
	
	for (int i = 0; i < 10; i++) {
		pintf();
	}
}