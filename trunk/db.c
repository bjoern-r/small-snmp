#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "snmp.h"
#include "db.h"

unsigned int num_digits(unsigned int num) {
    if (num < 10)
        return 1;

    return 1 + num_digits(num / 10);
}

struct oid_db *create_oid_db(void)
{
    struct oid_db *db = (struct oid_db *) malloc(sizeof(struct oid_db));
    db->length = 0;
    db->oids = NULL;

    return db;
}

int add_oid(struct oid_db *db, unsigned char *oid, unsigned char data_type, void *value)
{
    struct varbind *varbind = NULL;

    varbind = create_varbind(oid, data_type, value);

    /* Increase database _pointer_ range */
    db->oids = (struct varbind **) realloc(db->oids,
                sizeof(struct varbind *) * (db->length + 1));

    db->oids[db->length] = varbind;
    db->length++;

    /* Return index of added oid */
    return (int) db->length - 1;
}

int del_oid(struct oid_db *db, unsigned char *oid)
{
    int i;

    int del_idx = search_oid(db, oid);
    if(del_idx < 0) {
        printf("From del_oid(): Couldn't found oid %s\n", oid);
        return -1;
    }

    printf("Deleting %s (%d)\n", oid, del_idx);

    clr_varbind(db->oids[del_idx]);

    for(i = del_idx; i < db->length - 1; i++)
        db->oids[i] = db->oids[i + 1];

    db->length--;

    return db->length;
}


/* Return index of required oid */
int search_oid(struct oid_db *db, unsigned char *oid)
{
    unsigned int i;

    for(i = 0; i < db->length; i++) {
        if(!strcmp((char *) db->oids[i]->oid, (char *) oid))
            return i;
    }

    /* Not found */
    return -1;
}

int search_next_oid(struct oid_db *db, unsigned char *oid)
{
    regex_t regex;
    size_t nmatch = 2;
    regmatch_t match[2];
    int ret;
    int idx;

    /* Get minor oid part (witch will be incremented) */
    ret = regcomp(&regex, "^[[:digit:]\\.]+\\.([[:digit:]]+)$", REG_EXTENDED);
    if(ret) {
        printf("Could not compile regex\n");
        return -1;
    }

    ret = regexec(&regex, (const char *) oid, nmatch, match, 0);
    regfree(&regex);

    if(ret == REG_NOMATCH) {
        printf("No match\n");
        return -1;
    } else if(ret != 0) {
        /* Use regerror() to get regex error */
        printf("regexec execution error\n");
        return -1;
    }

    if(match[1].rm_so == -1) {
        printf("Substring match fail\n");
        return -1;
    }

    char *minor_str = malloc(match[1].rm_eo - match[1].rm_so + 1);
    memcpy(minor_str, (char *) oid + match[1].rm_so,
           match[1].rm_eo - match[1].rm_so);
    *(minor_str + match[1].rm_eo - match[1].rm_so) = '\0';

    int minor_dig = atoi(minor_str);
    free(minor_str);

    minor_dig++;

    unsigned int minor_dig_len = num_digits(minor_dig);

    minor_str = (char *) malloc(minor_dig_len + 1);
    snprintf(minor_str, minor_dig_len + 1, "%d", minor_dig);


    /* Get major oid part */
    ret = regcomp(&regex, "^([[:digit:]\\.]+\\.)[[:digit:]]+$", REG_EXTENDED);
    if(ret) {
        printf("Couldn't compile regex\n");
        return -1;
    }

    ret = regexec(&regex, (const char *) oid, (size_t) 2, match, 0);
    regfree(&regex);

    if(ret == REG_NOMATCH) {
        printf("No match\n");
        return -1;
    } else if(ret != 0) {
        /* Use regerror() to get regex error */
        printf("regexec execution error\n");
        return -1;
    }

    if(match[1].rm_so == -1) {
        printf("Substring match fail\n");
        return -1;
    }

    char *major_str = malloc(match[1].rm_eo - match[1].rm_so + 1);
    memcpy(major_str, (char *) oid + match[1].rm_so,
           match[1].rm_eo - match[1].rm_so);
    *(major_str + match[1].rm_eo - match[1].rm_so) = '\0';

    major_str = realloc(major_str, strlen(major_str) + strlen(minor_str) + 1);
    strcat(major_str, minor_str);
    free(minor_str);

    idx = search_oid(db, (unsigned char *) major_str);

    if(idx >= 0) {
        /* Replace oid found */
        oid = (unsigned char *) realloc(oid, strlen(major_str) + 1);
        memcpy(oid, major_str, strlen(major_str) + 1);
        free(major_str);

        return idx;
    }

    /* Else build next level oid (just add ".1" to original oid) */
    char *nl_oid = (char *) malloc(strlen((char *) oid) + 3); 

    memcpy(nl_oid, oid, strlen((char *)oid) + 1);
    strcat(nl_oid, ".1");

    idx = search_oid(db, (unsigned char *) nl_oid);

    if(idx >= 0) {
        /* Replace oid found */
        oid = (unsigned char *) realloc(oid, strlen(nl_oid) + 1);
        memcpy(oid, nl_oid, strlen(nl_oid) + 1);
        free(nl_oid);

        return idx;
    }

    /* Not found */
    return -1;
}


void print_oid_db(struct oid_db *db)
{
    unsigned int i;

    printf("==== OID DB ====\n");
    for(i = 0; i < db->length; i++) {
        switch(db->oids[i]->data_type) {
        case PRIMV_OCTSTR:
            printf("=%s=\t=%s=\n", (char *) db->oids[i]->oid, (char *) db->oids[i]->value);
            break;
        default:
            printf("=%s=\t=%d=\n", (char *) db->oids[i]->oid, *(int *) db->oids[i]->value);
        }
    }

}
