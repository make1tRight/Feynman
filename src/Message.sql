CREATE TABLE `message` (
    `id` BIGINT NOT NULL AUTO_INCREMENT COMMENT '自增id',
    `uuid` CHAR(20) NOT NULL UNIQUE COMMENT '消息uuid',
    `session_id` CHAR(20) NOT NULL COMMENT '会话uuid',
    `type` TINYINT NOT NULL COMMENT '消息类型: 0.文本, 1.文件, 2.通话',
    `content` TEXT COMMENT '消息内容',
    `url` CHAR(255) COMMENT 'url消息',
    `send_uid` CHAR(20) NOT NULL COMMENT '发送者uid',
    `send_name` VARCHAR(20) NOT NULL COMMENT 
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4_0900_ai_ci COMMENT='消息表'