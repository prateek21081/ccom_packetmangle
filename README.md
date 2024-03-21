# Covert Communication 

People Involved:
- Anirudh S. Kumar
- Prateek Kumar
- Paras Dhiman
- Mehul Arora
- Jithin S.
- Richa Gupta
- Sambuddho Chakravarty

## Setting up netfilter queue

### `iptables` rules
```bash
iptables -I INPUT -s 192.168.226.165 -d 192.168.226.176/32 -j NFQUEUE --queue-num 1 # destination is client and source is server
iptables -I OUTPUT -s 192.168.226.176/32 -d 192.168.226.165 -j NFQUEUE --queue-num 1 # server destination and client source
```

### `nftable` rules
Before inserting the rules, you also need to make the tables and make chains
```bash
nft add table inet filter 
nft add chain inet filter INPUT '{ type filter hook input priority 0 ; policy accept ; }'
nft add chain inet filter OUTPUT '{ type filter hook output priority 0 ; policy accept ; }'
```
Now you can insert the nft rules as following:-
```bash
nft insert rule inet filter INPUT ip saddr 192.168.226.165 ip daddr 192.168.226.176 counter queue num 1 
nft insert rule inet filter OUTPUT ip saddr 192.168.226.176 ip daddr 192.168.226.165 counter queue num 1 
```
