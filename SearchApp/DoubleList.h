#pragma once

#ifndef DOUBLELIST_H
#define DOUBLELIST_H


template <class CType>
struct DBLIST_ITEM
{
	typedef DBLIST_ITEM<CType>* Iterator;

	CType m_Value;


	DBLIST_ITEM()
	{
		pNext = pPrev = NULL;
	}

	DBLIST_ITEM* pNext, *pPrev;
};

template <class CType>
class CDoubleList
{
private:
	DBLIST_ITEM<CType>* pFirst, *pLast;

public:
	typedef DBLIST_ITEM<CType>* Iterator;
	typedef void (__stdcall *ONDESTROYITEM)(CType& Item);

	ONDESTROYITEM OnDestroyItem;

	void push_back(CType& item)
	{
		if (pFirst == NULL)
		{
			//if first was null, things will be like this: pFirst = pLast = value. for both, pNext, pPrev = NULL
			pFirst = new DBLIST_ITEM<CType>;
			pFirst->m_Value = item;
			pLast = pFirst;
		}
		else
		{
			DBLIST_ITEM<CType>* pItem = new DBLIST_ITEM<CType>;
			pItem->m_Value = item;
			//if this is second item: pLast = pFirst, and it will be First->pNext = pLast
			//if this is the third or more, it will be: pFirst->pNext = .. ->pNext = pLast;
			DBLIST_ITEM<CType>* pWasLast = pLast;
			pLast->pNext = pItem;
			pLast = pItem;
			pLast->pPrev = pWasLast;
		}
	}

	void push_front(CType& item)
	{
		if (pFirst == NULL)
		{
			//if first was null, things will be like this: pFirst = pLast = value. for both, pNext, pPrev = NULL
			pFirst = new DBLIST_ITEM<CType>;
			pFirst->m_Value = item;
			pLast = pFirst;
		}
		else
		{
			DBLIST_ITEM<CType>* pItem = new DBLIST_ITEM<CType>;
			pItem->m_Value = item;
			//if this is second item: pLast = pFirst, and it will be First->pNext = pLast
			//if this is the third or more, it will be: pFirst->pNext = .. ->pNext = pLast;
			DBLIST_ITEM<CType>* pWasFirst = pFirst;
			pFirst->pPrev = pItem;
			pFirst = pItem;
			pFirst->pNext = pWasFirst;
		}
	}

	Iterator pop_back()
	{
		if (is_empty()) return NULL;
		if (OnDestroyItem)
			OnDestroyItem(pLast->m_Value);

		DBLIST_ITEM<CType>* pItem = pLast->pPrev;
		delete pLast;
		pLast = pItem;

		if (pLast)
			pLast->pNext = NULL;
		else pFirst = NULL;

		return pLast;
	}

	Iterator pop_front()
	{
		if (is_empty()) return NULL;
		if (OnDestroyItem)
			OnDestroyItem(pFirst->m_Value);

		DBLIST_ITEM<CType>* pItem = pFirst->pNext;
		delete pFirst;
		pFirst = pItem;

		if (pFirst)
			pFirst->pPrev = NULL;
		else pLast = NULL;

		return pFirst;
	}

	int size()
	{
		int nr = 0;
		Iterator I;
		for (I = pFirst; I != NULL; )
		{
			I = I->pNext;
			nr++;
		}
		return nr;
	}

	bool is_empty()
	{
		return !pFirst;
	}

	Iterator begin()
	{
		return pFirst;
	}

	Iterator end()
	{
		return pLast;
	}

	CType& front()
	{
		return pFirst->m_Value;
	}

	CType& back()
	{
		return pLast->m_Value;
	}

	CType& operator[](int i)
	{
		Iterator X;
		int x;
		for (x = 0, X = pFirst; x < i; x++)
			X = X->pNext;

		return X->m_Value;
	}

	Iterator GetAt(int i)
	{
		Iterator X;
		int x;
		for (x = 0, X = pFirst; x < i; x++)
			X = X->pNext;

		return X;
	}

	void erase(Iterator& I)
	{
		//prev --- I --- next
		Iterator prev = I->pPrev;
		Iterator next = I->pNext;

		if (OnDestroyItem)
			OnDestroyItem(I->m_Value);
		delete I;

		I = next;

		if (prev)
			prev->pNext = next;
		//if I is now the first element
		else pFirst = next;

		if (next)
			next->pPrev = prev;
		//if I is now the last element
		else pLast = prev;
	}

	void erase(Iterator& From, Iterator& To)
	{
		//prev -- from  -- to -- next
		Iterator prev = From->pPrev;
		Iterator next = To->pNext;

		for (Iterator X = From; X != To->pNext;)
		{
			if (OnDestroyItem)
			{
				OnDestroyItem(X->m_Value);
			}

			Iterator aux = X->pNext;
			delete X;
			X = aux;
		}

		From = To = NULL;

		//here we have
		//-- prev -- from/to -- next
		if (prev)
			prev->pNext = next;
		//if  is now the first element
		else pFirst = next;

		if (next)
			next->pPrev = prev;
		//if I is now the last element
		else pLast = prev;
	}

	void insert_before(Iterator Where, CType& Value)
	{
		Iterator V = new DBLIST_ITEM<CType>;
		V->m_Value = Value;

		Iterator Prev = Where->pPrev;
		V->pPrev = Prev;
		Prev->pNext = V;

		Where->pPrev = V;
		V->pNext = Where;
	}

	void insert_after(Iterator Where, CType& Value)
	{
		Iterator V = new DBLIST_ITEM<CType>;
		V->m_Value = Value;

		Iterator Next = Where->pNext;
		V->pPrev = Where;
		Where->pNext = V;

		Next->pPrev = V;
		V->pNext = Next;
	}

	void erase_all()
	{
		while (pop_back());
	}

	void reverse()
	{
		Iterator I, J;
		CType value;

		for (I = pFirst, J = pLast; I < J; I = I->pNext, J = J->pPrev)
		{
			value = I->m_Value;
			I->m_Value = J->m_Value;
			J->m_Value = value;
		}
	}

	CDoubleList(ONDESTROYITEM OnDestroy)
	{
		OnDestroyItem = OnDestroy;
		pFirst = pLast = NULL;
	}

	~CDoubleList(void)
	{
		while (pop_back());
	}
};

#endif//DOUBLELIST_H