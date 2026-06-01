-- AngelDB Character Services Tables
-- Migrated from: sql/custom/characters/2026_05_13_00_characters_battlepay.sql

-- Guild Services (name change, faction change, transfer)
CREATE TABLE IF NOT EXISTS `battlepay_guild_services` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `guild_id` bigint(20) unsigned NOT NULL DEFAULT 0,
  `service_type` varchar(50) NOT NULL DEFAULT 'namechange' COMMENT 'namechange, factionchange, transfer',
  `account_id` int(10) unsigned NOT NULL DEFAULT 0,
  `status` tinyint(3) unsigned NOT NULL DEFAULT 0 COMMENT '0=pending, 1=processing, 2=completed',
  `requested_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `completed_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_guild_service` (`guild_id`, `service_type`),
  KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
