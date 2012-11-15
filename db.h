#ifndef DB_H_INCLUDED
#define DB_H_INCLUDED

struct oid_db {
    unsigned int length;
    struct varbind **oids;
};

struct oid_db *create_oid_db(void);
int add_oid(struct oid_db *db, unsigned char *oid, unsigned char data_type, void *value);
int del_oid(struct oid_db *db, unsigned char *oid);
int search_oid(struct oid_db *db, unsigned char *oid);
int search_next_oid(struct oid_db *db, unsigned char *oid);
void print_oid_db(struct oid_db *db);

#endif
