const char *symb_repr(uint16);
uint32 embedded_symbs_count();

OBJ     opt_repr_build(OBJ tag, OBJ value);
uint16  opt_repr_get_tag_id(uint16);
bool    opt_repr_may_have_opt_fields(uint16 repr_id);
bool    opt_repr_has_field(void*, uint16, uint16);
OBJ     opt_repr_lookup_field(void *ptr, uint16 repr_id, uint16 field_id);
uint32  opt_repr_get_fields_count(void *ptr, uint16 repr_id);
uint16 *opt_repr_get_fields(uint16 repr_id, uint32 &count);
void   *opt_repr_copy(void *ptr, uint16 repr_id);
uint32  opt_repr_mem_size(void *ptr, uint16 repr_id);
void    opt_repr_copy_to_pool(void *ptr, uint16 repr_id, void **dest_var);
int     opt_repr_cmp(void *ptr1, void *ptr2, uint16 repr_id);
