bool    opt_repr_has_field(void*, uint16, uint16);
OBJ     opt_repr_lookup_field(void *ptr, uint16 repr_id, uint16 field_id);
uint32  opt_repr_get_fields_count(void *ptr, uint16 repr_id);
uint16 *opt_repr_get_fields(void *ptr, uint16 repr_id, uint32 &count);