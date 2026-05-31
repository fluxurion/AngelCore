-- AngelDB BattlePay Transaction Tables
-- Migrated from: sql/custom/auth/2026_05_13_00_auth_battlepay.sql
--                sql/custom/characters/2026_05_13_00_characters_battlepay.sql

-- BattlePay credits per account (replaces ALTER TABLE account ADD battlepay_credits)
CREATE TABLE IF NOT EXISTS `battlepay_credits` (
  `account_id` int(10) unsigned NOT NULL,
  `credits`    int(10) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- BattlePay purchases table
CREATE TABLE IF NOT EXISTS `battlepay_purchases` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int(10) unsigned NOT NULL DEFAULT 0,
  `product_id` int(10) unsigned NOT NULL DEFAULT 0,
  `status` tinyint(3) unsigned NOT NULL DEFAULT 0 COMMENT '0=pending, 1=completed, 2=failed',
  `result_code` int(10) unsigned NOT NULL DEFAULT 0,
  `wallet_name` varchar(255) DEFAULT 'BattlePay',
  `player_guid` bigint(20) unsigned DEFAULT 0,
  `client_send` tinyint(1) DEFAULT 0,
  `revoked` tinyint(1) DEFAULT 0,
  `unk_guid` bigint(20) unsigned DEFAULT 0,
  `unk_int1` int(10) unsigned DEFAULT 0,
  `unk_int2` int(10) unsigned DEFAULT 0,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_account` (`account_id`),
  KEY `idx_product` (`product_id`),
  KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- BattlePay distributions table (offline deliveries)
CREATE TABLE IF NOT EXISTS `battlepay_distributions` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `purchase_id` bigint(20) unsigned NOT NULL DEFAULT 0,
  `product_id` int(10) unsigned NOT NULL DEFAULT 0,
  `account_id` int(10) unsigned NOT NULL DEFAULT 0,
  `status` tinyint(3) unsigned NOT NULL DEFAULT 0 COMMENT '0=pending, 1=delivered',
  `target_guid` bigint(20) unsigned DEFAULT 0,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_purchase` (`purchase_id`),
  KEY `idx_account` (`account_id`),
  KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- BattlePay pending rewards (offline character delivery)
CREATE TABLE IF NOT EXISTS `battlepay_pending_rewards` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `account_id` int(10) unsigned NOT NULL DEFAULT 0,
  `character_guid` bigint(20) unsigned NOT NULL DEFAULT 0,
  `reward_type` varchar(50) NOT NULL DEFAULT 'item' COMMENT 'item, mount, toy, transmog, service, gold',
  `reward_id` int(10) unsigned NOT NULL DEFAULT 0,
  `reward_amount` int(10) unsigned DEFAULT 1,
  `status` tinyint(3) unsigned NOT NULL DEFAULT 0 COMMENT '0=pending, 1=delivered, 2=failed',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `delivered_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_account` (`account_id`),
  KEY `idx_character` (`character_guid`),
  KEY `idx_status` (`status`),
  KEY `idx_type` (`reward_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
