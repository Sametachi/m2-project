#pragma once

class CUndoBuffer
{
	public:
		class IUndoData
		{
			public:
				IUndoData(){}
				virtual ~IUndoData(){}

				virtual void Backup() = 0;
				virtual void Restore() = 0;

				virtual void BackupStatement() = 0;
				virtual void RestoreStatement() = 0;
		};
		typedef std::deque<IUndoData*>		TUndoDataDeque;
		typedef TUndoDataDeque::iterator	TUndoDataIterator;

	public:
		CUndoBuffer();
		~CUndoBuffer();

		void ClearTail(uint32_t dwIndex);

		void Backup(IUndoData * pData);
		void BackupCurrent(IUndoData * pData);

		void Undo();
		void Redo();

	protected:
		bool GetUndoData(uint32_t dwIndex, IUndoData ** ppUndoData);

	protected:
		uint32_t m_dwCurrentStackPosition;
		TUndoDataDeque m_UndoDataDeque;

		IUndoData * m_pTopData;

		const uint8_t m_ucMAXBufferCount;
};