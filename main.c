#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define EFI_VAR_NAME_LEN 1024
#define EFI_DATA_MAXLEN 1024

struct guid {
	uint32_t a;
	uint16_t b;
	uint16_t c;
	uint8_t d[8];
};

#define EFI_GLOBAL_VARIABLE_GUID (struct guid){ \
	0x8be4df61, 0x93ca, 0x11d2, \
	{ 0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c } }

#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI 1

#define EFI_VARIABLE_NON_VOLATILE 1
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 2
#define EFI_VARIABLE_RUNTIME_ACCESS 4

int main(int argc, char* argv[]) {
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		fprintf(stderr, "\n");
		fprintf(stderr, "Sets firmware UI to open at next boot.\n");
		return 1;
	}

	char * name = "OsIndications";

	struct guid vendor_guid = EFI_GLOBAL_VARIABLE_GUID;

	uint64_t data = EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
	unsigned long data_size = sizeof(data);

	unsigned long status = 0;

	uint32_t attributes =
		EFI_VARIABLE_NON_VOLATILE |
		EFI_VARIABLE_BOOTSERVICE_ACCESS |
		EFI_VARIABLE_RUNTIME_ACCESS;

	uint8_t buf[
		EFI_VAR_NAME_LEN +
		sizeof(vendor_guid) +
		sizeof(data_size) +
		EFI_DATA_MAXLEN +
		sizeof(status) +
		sizeof(attributes)] = { 0 };

	char efivars_dirname[
		(sizeof("/sys/firmware/efi/vars/") - 1) +
		(EFI_VAR_NAME_LEN / 2) +
		(sizeof("-") - 1) +
		(sizeof("XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX") - 1) +
		(sizeof("/raw_var") - 1) +
		1];
	sprintf(
		efivars_dirname,
		"/sys/firmware/efi/vars/"
			"%s"
			"-"
			"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
			"/raw_var",
		name,
		vendor_guid.a,
		vendor_guid.b,
		vendor_guid.c,
		vendor_guid.d[0],
		vendor_guid.d[1],
		vendor_guid.d[2],
		vendor_guid.d[3],
		vendor_guid.d[4],
		vendor_guid.d[5],
		vendor_guid.d[6],
		vendor_guid.d[7]);

	int efi_data_fd = open(efivars_dirname, O_RDWR);
	if (efi_data_fd == -1 && errno != ENOENT) {
		perror("open EFI variable failed");
		return 1;
	}

	if (efi_data_fd != -1) {
		ssize_t ret = read(efi_data_fd, buf, sizeof(buf));
		if (ret < 0 || (size_t)ret != sizeof(buf)) {
			perror("read EFI variable failed");
			return 1;
		}

		uint8_t * buf_ptr = buf;
		buf_ptr += EFI_VAR_NAME_LEN;
		buf_ptr += sizeof(vendor_guid);
		memcpy(buf_ptr, &data_size, sizeof(data_size));
		buf_ptr += sizeof(data_size);
		memcpy(buf_ptr, &data, sizeof(data));
		buf_ptr += sizeof(data);
		memset(buf_ptr, 0, EFI_DATA_MAXLEN - sizeof(data));
		buf_ptr += EFI_DATA_MAXLEN - sizeof(data);
	} else {
		efi_data_fd = open("/sys/firmware/efi/vars/new_var", O_WRONLY);
		if (efi_data_fd == -1) {
			perror("open new EFI variable failed");
			return 1;
		}

		uint8_t * buf_ptr = buf;
		for (size_t i = 0; name[i] != '\0'; i++) {
			buf_ptr[i * 2] = (uint8_t)name[i];
			buf_ptr[i * 2 + 1] = 0;
		}
		buf_ptr += EFI_VAR_NAME_LEN;
		memcpy(buf_ptr, &vendor_guid, sizeof(vendor_guid));
		buf_ptr += sizeof(vendor_guid);
		memcpy(buf_ptr, &data_size, sizeof(data_size));
		buf_ptr += sizeof(data_size);
		memcpy(buf_ptr, &data, sizeof(data));
		buf_ptr += EFI_DATA_MAXLEN;
		memcpy(buf_ptr, &status, sizeof(status));
		buf_ptr += sizeof(status);
		memcpy(buf_ptr, &attributes, sizeof(attributes));
		buf_ptr += sizeof(attributes);
	}

	ssize_t ret = write(efi_data_fd, buf, sizeof(buf));
	if (ret < 0 || (size_t)ret != sizeof(buf)) {
		perror("write EFI variable failed");
		return 1;
	}

	fprintf(stdout, "set EFI variable, now reboot to access firmware UI\n");
	return 0;
}
