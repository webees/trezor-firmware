from trezor.crypto import monero as tcry

from apps.monero.xmr.networks import NetworkTypes, net_version


def addr_to_hash(addr):
    """
    Creates hashable address representation
    """
    return bytes(addr.spend_public_key + addr.view_public_key)


def encode_addr(version, spend_pub, view_pub, payment_id=None):
    """
    Encodes public keys as versions
    """
    buf = spend_pub + view_pub
    if payment_id:
        buf += bytes(payment_id)
    return tcry.xmr_base58_addr_encode_check(ord(version), bytes(buf))


def decode_addr(addr):
    """
    Given address, get version and public spend and view keys.
    """
    d, version = tcry.xmr_base58_addr_decode_check(bytes(addr))
    pub_spend_key = d[0:32]
    pub_view_key = d[32:64]
    return version, pub_spend_key, pub_view_key


def public_addr_encode(pub_addr, is_sub=False, net=NetworkTypes.MAINNET):
    """
    Encodes public address to Monero address
    """
    net_ver = net_version(net, is_sub)
    return encode_addr(net_ver, pub_addr.spend_public_key, pub_addr.view_public_key)


def classify_subaddresses(tx_dests, change_addr):
    """
    Classify destination subaddresses
    """
    num_stdaddresses = 0
    num_subaddresses = 0
    single_dest_subaddress = None
    addr_set = set()
    for tx in tx_dests:
        if change_addr and addr_eq(change_addr, tx.addr):
            continue
        addr_hashed = addr_to_hash(tx.addr)
        if addr_hashed in addr_set:
            continue
        addr_set.add(addr_hashed)
        if tx.is_subaddress:
            num_subaddresses += 1
            single_dest_subaddress = tx.addr
        else:
            num_stdaddresses += 1
    return num_stdaddresses, num_subaddresses, single_dest_subaddress


def addr_eq(a, b):
    return (
        a.spend_public_key == b.spend_public_key
        and a.view_public_key == b.view_public_key
    )


def get_change_addr_idx(outputs, change_dts):
    """
    Returns ID of the change output from the change_dts and outputs
    """
    if change_dts is None:
        return None

    change_idx = None
    for idx, dst in enumerate(outputs):
        if (
            change_dts.amount
            and change_dts.amount == dst.amount
            and addr_eq(change_dts.addr, dst.addr)
        ):
            change_idx = idx
    return change_idx
