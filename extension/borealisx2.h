#define BOREALISX2_PUBLICATION_PREFIX "pub_"
#define BOREALISX2_SUBSCRIPTION_PREFIX "sub_"

typedef struct Borealisx2Node
{
    char *node_id;
    char *node_host;
    char *node_remote_conn_dsn;
    char *node_status;
} Borealisx2Node;

int borealisx2_utils_spi_getattno(const char *colname);
void borealisx2_utils_sysid_get(char *sys_id);