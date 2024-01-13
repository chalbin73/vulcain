#include "fio.h"

void   *fio_read_whole_file(const char *filepath, u64 *file_size)
{
    FILE *f = fopen(filepath, "r");
    if (!f)
    {
        return NULL;
    }

    // Get file length
    fseek(f, 0, SEEK_END);
    *file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Allocate necessary memory
    void *data = mem_allocate(*file_size, MEMORY_TAG_FIO_DATA);

    fread(data, *file_size, 1, f);

    fclose(f);

    return data;
}

