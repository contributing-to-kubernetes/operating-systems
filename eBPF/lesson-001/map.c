#include <linux/bpf.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

// Include definitions for map helper utilities for user-space.
#include "bpf.h"
// Include map amp_update_elem utility for kernel.
//#include "bpf_helpers.h"  // This is found in
// tools/testing/selftests/bpf/bpf_helpers.h.

/*

   Here are some of the ways to create a map:

union bpf_attr my_map {
  .map_type = BPF_MAP_TYPE_HASH,
  .key_size = sizeof(int),
  .value_size = sizeof(int),
  .max_entries = 100,
  .map_flags = BPF_F_NO_PREALLOC,
};

int fd = bpf(BPF_MAP_CREATE, &my_map, sizeof(my_map));


int fd;
fd = bpf_create_map(BPF_MAP_TYPE_HASH, sizeof(int), sizeof(int), 100, BPF_F_NO_PREALLOC);


struct bpf_map_def sec("maps") my_map = {
  .type = BPF_MAP_TYPE_HASH,
  .key_size = sizeof(int),
  .value_size = sizeof(int),
  .max_entries = 100,
  .map_flags = BPF_F_NO_PREALLOC,
};

fd = map_data[0].fd;
*/

int main(int argc, char **argv) {
  /*  Creating a map
   *
   * Let's get this thing rolling by creating a map. We will use the user-space
   * helper functions which require the first argument to be a file descriptor.
   */
  int fd;
  fd = bpf_create_map(BPF_MAP_TYPE_HASH, sizeof(int), sizeof(int), 100, BPF_F_NO_PREALLOC);
  if (fd < 0) {
    printf("failed to create map: %d (%s)\n", fd, strerror(errno));
    return -1;
  }

  int key, value, result;

  /*  Updating map entries
   */
  // BPF_ANY updates the given value if it exists, or creates an entry for the
  // given key if it doesn't exist.
  key = 1, value = 1234;
  result = bpf_map_update_elem(fd, &key, &value, BPF_ANY);
  if (result == 0)
    printf("map updated with new element\n");
  else
    printf("failed to update map with new value: %d (%s)\n", result, strerror(errno));

  // BPF_NOEXIST creates an entry for the given key only if it doesn't exist.
  key = 1, value = 5678;
  result = bpf_map_update_elem(fd, &key, &value, BPF_NOEXIST);
  if (result == 0)
    printf("map updated with new element\n");
  else
    printf("failed to update map with new value: %d (%s)\n", result, strerror(errno));

  // BPF_EXIST updates the value for the given key only if it exists.
  key = 1234, value = 5678;
  result = bpf_map_update_elem(fd, &key, &value, BPF_EXIST);
  if (result == 0)
    printf("map updated with new element\n");
  else
    printf("failed to update map with new values: %d (%s)\n", result, strerror(errno));

  /*  Look up map elements
   *
   *  Now that we created a map and created some entries, let's try to retrieve
   *  them.
   */
  int map_value;
  key = 1;
  result = bpf_map_lookup_elem(fd, &key, &map_value);
  if (result == 0)
    printf("value read from the map: '%d'\n", map_value);
  else
    printf("failed to read value from the map: %d (%s)\n", result, strerror(errno));

  /*  Delete map entries
   */
  result = bpf_map_delete_elem(fd, &key);
  if (result == 0)
    printf("element deleted from map\n");
  else
    printf("failed to delete element from the map: %d (%s)\n", result, strerror(errno));

  /*
   * The following method or iterating through a map uses the helper function
   * `bpf_map_get_next_key`, which doesn't have a kernel analog.
   */
  int new_key, new_value;
  for (int it=1; it<5; it++) {
    new_key = it;
    new_value = 1234 + it;
    bpf_map_update_elem(fd, &new_key, &new_value, BPF_NOEXIST);
  }

  // Now to print all map entries.
  // Here we will use a map key that doesn't exist - bpf_map_get_next_key
  // will start from the begining of the map.
  // The error value of this function will be ENOENT.
  int lookup_key, next_key;
  lookup_key = -1;
  while (bpf_map_get_next_key(fd, &lookup_key, &next_key) == 0) {
    printf("the next key in the map is: '%d'\n", next_key);
    lookup_key = next_key;
  }
  printf("let's try deleting a value while iterating...\n\n");

  // Some cool and interesting behaviour of bpf_map_get_next_key? If you delete
  // a key while iterating over the map it seems to force it to start over.
  lookup_key = -1;
  while (bpf_map_get_next_key(fd, &lookup_key, &next_key) == 0) {
    printf("the next key in the map is: '%d'\n", next_key);

    if (next_key == 2) {
      printf("deleting key %d\n", next_key);
      bpf_map_delete_elem(fd, &next_key);
    }

    lookup_key = next_key;
  }

  // On the other hand, if you want to look up an element and delete it, you
  // can use `bpf_map_lookup_and_delete_elem`.
  //
  // Note: this may return an errno 524 (ENOTSUPP - not supported) based on
  // your kernel version.
  key = 1;
  for (int it=0; it<2; it++) {
    result = bpf_map_lookup_and_delete_elem(fd, &key, &value);
    if (result == 0)
      printf("value from the map: '%d'\n", value);
    else
      printf("failed to read value from map: %d (%s)\n", result, strerror(errno));
  }

  return 0;
}
