#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <hiredis-vip/hircluster.h>

const char *keys[] = {
    "key-1", "key-2", "key-3", "key-4", "key-5",
    "key-6", "key-7", "key-8", "key-9", "key-10"
};

const char *values[] = {
    "value-1", "value-2", "value-3", "value-4", "value-5",
    "value-6", "value-7", "value-8", "value-9", "value-10"
};

void get_password(char *buf, int cap)
{
    fputs("Enter the password of the redis-server. "
            "Hit 'enter' to bypass.\nPassword: ", stdout);
    fgets(buf, cap, stdin);

    int len = strlen(buf);

    if (buf[len-1] == '\n') {
        buf[len-1] = '\0';
    }

    printf("\n");
}

void test_dist_keys(redisClusterContext *cc)
{
    printf("Test set/get keys spreading over a cluster...\n");

    for (int i = 0; i < 10; ++i) {
        redisReply *reply = redisClusterCommand(cc, "set %s %s", keys[i], values[i]);

        if (reply == NULL) {
            printf("reply is null, err[%s]\n", cc->errstr);
            break;
        }

        freeReplyObject(reply);
        reply = redisClusterCommand(cc, "get %s", keys[i]);

        if (reply == NULL) {
            printf("reply is null, err[%s]\n", cc->errstr);
            break;
        }

        printf("reply->str: %s\n", reply->str);
        freeReplyObject(reply);
    }

    printf("\n");
}

void test_dist_keys_loop(redisClusterContext *cc)
{
    printf("Test fetching keys over and over...\n");

    while (1) {
        test_dist_keys(cc);
        sleep(1);
    }
}

void test_lua_scripting(redisClusterContext *cc)
{
    printf("Test simple lua scripting...\n");

    const char *lua_cmd = "return redis.call('get',KEYS[1])..', exp='..ARGV[1]";
    redisReply *reply = redisClusterCommand(cc, "eval %s 1 %s %s", lua_cmd, keys[0], values[0]);

    if (reply == NULL) {
        printf("reply is null, err[%s]\n", cc->errstr);
    }

    printf("reply->str: %s\n", reply->str);
    freeReplyObject(reply);

    printf("\n");
}

void test_pipeline(redisClusterContext *cc)
{
    printf("Test pipelining...\n");

    const char *pipelined_cmds[] = {
        "set pipelined_key value-123",
        "get pipelined_key",
        "set pipelined_key value-456",
        "get pipelined_key"
    };

    for (int i = 0; i < 4; ++i) {
        redisClusterAppendCommand(cc, pipelined_cmds[i]);
    }

    redisReply *reply;

    for (int i = 0; i < 4; ++i) {
        redisClusterGetReply(cc, (void**)(&reply));

        if (reply == NULL) {
            printf("reply is null, err[%s]\n", cc->errstr);
            break;
        }

        if (i % 2 != 0) {
            printf("reply->str: %s\n", reply->str);
        }

        freeReplyObject(reply);
    }

    redisClusterReset(cc);

    printf("\n");
}

redisClusterContext* create_cluster_context()
{
	redisClusterContext *cc = NULL;
	cc = redisClusterContextInit();

    /* No need to configure all cluster nodes. To provide a few samples is fine.*/
	redisClusterSetOptionAddNodes(cc, "127.0.0.1:7000");

    char password[128] = {0};
    get_password(password, sizeof(password));

    if (strlen(password) > 0) {
        /* Test authentication to cluster nodes. */
        redisClusterSetOptionAuth(cc, password);
    }

	redisClusterConnect2(cc);

	if (cc == NULL || cc->err) {
		printf("Error: %s\n", cc == NULL ? "NULL" : cc->errstr);
		exit(-1);
	}

    return cc;
}

int main(int argc, char *argv[])
{
    redisClusterContext *cc = create_cluster_context();

    test_dist_keys(cc);
    test_lua_scripting(cc);
    test_pipeline(cc);
    test_dist_keys_loop(cc);

    redisClusterFree(cc);
    return 0;
}
