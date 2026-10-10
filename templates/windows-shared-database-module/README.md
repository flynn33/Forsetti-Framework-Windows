# Windows Shared Database Module Template

This is a **manifest-only starter** for a default shared database provider module. No service target, provider implementation, or database engine is supplied.

Author and validate an app-owned provider through the installed public `Forsetti::ISharedDatabaseService` contract, using constructor-injected collaborators, before registering or activating this module. The manifest alone does not provide database functionality.
