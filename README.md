## Redis Cluster client demo

`
	//
	// 1) Create a redis-cluster context.
	//
    redisClusterContext *cc = NULL;
    cc = redisClusterContextInit();

	//
	// 1.1) Specify a few nodes of the cluster.
    //      No need to configure all cluster nodes.
    //      Providing a few samples is fine.
	//
    redisClusterSetOptionAddNodes(cc, "127.0.0.1:7000");

	//
	// 1.2) Specify password as needed
	//
    char password[128] = {0};
    get_password(password, sizeof(password));

    if (strlen(password) > 0) {
        /* Test authentication to cluster nodes. */
        redisClusterSetOptionAuth(cc, password);
    }

	//
	// 1.3) Issue connection
	//
    redisClusterConnect2(cc);
    if (cc == NULL || cc->err) {
        printf("Error: %s\n", cc == NULL ? "NULL" : cc->errstr);
        return -1;
    }

	//
	// 2) Test a few keys spreading across the cluster.
	//
    char *keys[] = {
        "key-1", "key-2", "key-3", "key-4", "key-5",
        "key-6", "key-7", "key-8", "key-9", "key-10"
    };

    char *values[] = {
        "value-1", "value-2", "value-3", "value-4", "value-5",
        "value-6", "value-7", "value-8", "value-9", "value-10"
    };

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

	//
	// 3) Test simple lua scripting.
	//
    const char *lua_cmd = "return redis.call('get',KEYS[1])..', exp='..ARGV[1]";
    redisReply *reply = redisClusterCommand(cc, "eval %s 1 %s %s", lua_cmd, keys[0], values[0]);

    if (reply == NULL) {
        printf("reply is null, err[%s]\n", cc->errstr);
    }

    printf("reply->str: %s\n", reply->str);
    freeReplyObject(reply);

	//
	// 4) Test pipelining.
	//
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

	//
	// 5) Clean exit.
	//
    redisClusterFree(cc);
`
