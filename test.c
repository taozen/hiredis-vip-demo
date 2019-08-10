#include <stdio.h>
#include <stdlib.h>

#include <hiredis-vip/hircluster.h>

void get_password(char *buf, int cap)
{
    fputs("Enter the password of the redis-server. "
            "Hit 'enter' to bypass.\nPassword: ", stdout);
    fgets(buf, cap, stdin);

    int len = strlen(buf);

    if (buf[len-1] == '\n') {
        buf[len-1] = '\0';
    }
}

int main(int argc, char *argv[])
{
    char *keys[] = {
        "key-1", "key-2", "key-3", "key-4", "key-5",
        "key-6", "key-7", "key-8", "key-9", "key-10"
    };

    char *values[] = {
        "value-1", "value-2", "value-3", "value-4", "value-5",
        "value-6", "value-7", "value-8", "value-9", "value-10"
    };

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
		return -1;
	}

    /* Test set/get keys spreading over a cluster. */
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

    /* Test simple lua scripting. */
    const char *lua_cmd = "return redis.call('get',KEYS[1])..', exp='..ARGV[1]";
    redisReply *reply = redisClusterCommand(cc, "eval %s 1 %s %s", lua_cmd, keys[0], values[0]);

    if (reply == NULL) {
        printf("reply is null, err[%s]\n", cc->errstr);
    }

    printf("reply->str: %s\n", reply->str);
    freeReplyObject(reply);

    /* Test pipelining. */
    const char *pipelined_cmds[] = {
        "set pipelined_key value-123",
        "get pipelined_key",
        "set pipelined_key value-456",
        "get pipelined_key"
    };

    for (int i = 0; i < 4; ++i) {
        redisClusterAppendCommand(cc, pipelined_cmds[i]);
    }

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
    redisClusterFree(cc);
    return 0;
}
