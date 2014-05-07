
class myvec{
public:
	int x, y;
	myvec(int x, int y){x = x; y = y;};
};


inline myvec operator + (const myvec& op1, const myvec& op2){
	return myvec(op1.x+op2.x, op2.y+op2.y);
}


