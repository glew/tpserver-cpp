
#include "order.h"
#include "frame.h"

#include "move.h"

Move::Move()
{
	type = odT_Move;
}

Move::~Move()
{

}

long long Move::getX()
{
	return x;
}

long long Move::getY()
{
	return y;
}

long long Move::getZ()
{
	return z;
}

void Move::setDest(long long x1, long long y1, long long z1)
{
	x = x1;
	y = y1;
	z = z1;
}

void Move::createFrame(Frame * f, int objID, int pos)
{
	Order::createFrame(f, objID, pos);
	f->packInt64(x);
	f->packInt64(y);
	f->packInt64(z);
}

void Move::inputFrame(Frame * f)
{
	x = f->unpackInt64();
	y = f->unpackInt64();
	z = f->unpackInt64();
}

void Move::createOutcome(Frame * f, int objID, int pos)
{
	Order::createOutcome(f, objID, pos);
	f->packInt(1);
}

void Move::describeOrder(int orderType, Frame * f)
{
	if (orderType == odT_Move) {
		f->packInt(odT_Move);
		f->packString("Move");
		f->packString("Move to a given position absolute in space");
		f->packInt(1);
		f->packString("Position");
		f->packInt(opT_Space_Coord);
		f->packString("The position in space to move to");
	}
}
