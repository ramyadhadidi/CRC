WORKLOADS=(mcf bwaves bzip2 zeusmp cactus gems sphinx various1 various2 various3)
WORKLOADS_L=${#WORKLOADS[@]}

LLCrepl="drrip"
LLCreplNum=2
iCount=200

for ((i=0;i<$WORKLOADS_L;i++)); do
	echo "/net/tinker/rhadidi6/CRC/bin/CMPsim.usetrace.64 -threads 4 -mix /net/tinker/rhadidi6/CRC/CRC_traces/mix_${WORKLOADS[${i}]}.mix -cache UL3:4096:64:16 -autorewind 1 -icount ${iCount} -o /net/tinker/rhadidi6/CRC/runs/mix_${WORKLOADS[${i}]}_${LLCrepl}.stats -LLCrepl ${LLCreplNum}"
done