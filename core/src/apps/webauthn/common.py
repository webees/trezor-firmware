from micropython import const

# CBOR object signing and encryption algorithms and keys
COSE_ALG_KEY = const(3)
COSE_ALG_ES256 = const(-7)  # ECDSA with SHA-256
COSE_ALG_EDDSA = const(-8)  # EdDSA
COSE_ALG_ECDH_ES_HKDF_256 = const(-25)  # Ephemeral-static ECDH with HKDF SHA-256
COSE_KEY_TYPE_KEY = const(1)
COSE_KEY_TYPE_OKP = const(1)  # octet key pair
COSE_KEY_TYPE_EC2 = const(2)  # elliptic curve keys with x- and y-coordinate pair
COSE_CURVE_KEY = const(-1)  # elliptic curve identifier
COSE_CURVE_P256 = const(1)  # P-256 curve
COSE_CURVE_ED25519 = const(6)  # Ed25519 curve
COSE_X_COORD_KEY = const(-2)  # x coordinate of the public key
COSE_Y_COORD_KEY = const(-3)  # y coordinate of the public key
