/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP

#include <string>
#include <stdexcept>
#include <vector>
#include <cmath>

using namespace std;

long long int intCast(double);
std::string toString(long long int);


/////////////////////////////////////////////////
/// \brief This class abstracts all the index
/// logics, i.e. the logical differences between
/// single indices and indices described by a
/// vector.
/////////////////////////////////////////////////
class VectorIndex
{
    private:
        mutable vector<long long int> vStorage;
        bool expand;

        /////////////////////////////////////////////////
        /// \brief This private memnber function
        /// calculates the index corresponding to the
        /// selected position n. Otherwise simply return
        /// the n-th vector index.
        ///
        /// \param n size_t
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        inline long long int getIndex(size_t n) const
        {
            // If single indices are used, they will be expanded
            // into virtual vectors
            if (expand)
            {
                // If the second index has not been defined,
                // we'll expand the index using no end
                if (vStorage.back() == OPEN_END)
                {
                    if (vStorage.size() == 2)
                        return vStorage.front() + n;
                    else if (vStorage.size() > n+1)
                        return vStorage[n];
                    else
                        return vStorage[vStorage.size()-1] + n - (vStorage.size()-1);
                }

                // Consider the case that the order of the indices
                // could be inverted
                if (vStorage.front() <= vStorage.back() && n + vStorage.front() <= vStorage.back())
                    return vStorage.front() + n;
                else if (vStorage.front() > vStorage.back() && vStorage.front() - n >= vStorage.back()) // >= because vStorage.back() was not decremented first
                    return vStorage.front() - n;
            }
            else if (n < vStorage.size())
                return vStorage[n];

            return INVALID;
        }

    public:
        enum
        {
            INVALID = -1,
            OPEN_END = -2,
            STRING = -3
        };

        /////////////////////////////////////////////////
        /// \brief Default constructor.
        ///
        ///
        /////////////////////////////////////////////////
        VectorIndex()
        {
            vStorage.assign({INVALID, INVALID});
            expand = true;
        }

        /////////////////////////////////////////////////
        /// \brief Copy constructor.
        ///
        /// \param vIndex const VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex(const VectorIndex& vIndex) : vStorage(vIndex.vStorage), expand(vIndex.expand)
        {
        }

        /////////////////////////////////////////////////
        /// \brief VectorIndex move constructor.
        ///
        /// \param vIndex VectorIndex&&
        ///
        /////////////////////////////////////////////////
        VectorIndex(VectorIndex&& vIndex) : expand(std::move(vIndex.expand))
        {
			std::swap(vStorage, vIndex.vStorage);
        }

        /////////////////////////////////////////////////
        /// \brief Constructor from an array of doubles.
        /// The third argument is used only to avoid
        /// misinterpretation from the compiler.
        ///
        /// \param indices const double*
        /// \param nResults int
        /// \param unused int
        ///
        /////////////////////////////////////////////////
        VectorIndex(const double* indices, int nResults, int unused)
        {
            // Store the indices and convert them to integers
            // using the intCast() function
            for (int i = 0; i < nResults; i++)
            {
                if (!isnan(indices[i]) && !isinf(indices[i]))
                    vStorage.push_back(intCast(indices[i]) - 1);
            }

            expand = false;
        }

        /////////////////////////////////////////////////
        /// \brief Constructor for single indices.
        ///
        /// \param nStart long long int
        /// \param nEnd long long int
        ///
        /////////////////////////////////////////////////
        VectorIndex(long long int nStart, long long int nEnd = INVALID)
        {
            vStorage.assign({nStart, nEnd});
            expand = true;
        }

        /////////////////////////////////////////////////
        /// \brief Constructor from a STL vector.
        ///
        /// \param vIndex const vector<long long int>&
        ///
        /////////////////////////////////////////////////
        VectorIndex(const vector<long long int>& vIndex)
        {
            if (vIndex.size())
            {
                vStorage = vIndex;
                expand = false;
            }
            else
            {
                vStorage.assign({INVALID, INVALID});
                expand = true;
            }
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload for the
        /// same type.
        ///
        /// \param vIndex const VectorIndex&
        /// \return VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex& operator=(const VectorIndex& vIndex)
        {
            vStorage = vIndex.vStorage;
            expand = vIndex.expand;
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload for STL
        /// vectors.
        ///
        /// \param vIndex const vector<long long int>&
        /// \return VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex& operator=(const vector<long long int>& vIndex)
        {
            if (vIndex.size())
            {
                vStorage = vIndex;
                expand = false;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a subset
        /// of the internal stored index just like the
        /// std::string::substr() member function.
        ///
        /// Single indices are not expanded therefore
        /// the storage space used for the newly created
        /// index won't be larger than the current one.
        /// However, already expanded indices will result
        /// in an expanded subindex vector.
        ///
        /// \note A length of 0 or 1 will both create a
        /// VectorIndex with only one valid entry (the
        /// first).
        ///
        /// \param pos size_t
        /// \param nLen size_t
        /// \return VectorIndex
        ///
        /////////////////////////////////////////////////
        VectorIndex subidx(size_t pos, size_t nLen = string::npos) const
        {
            // Consider some strange border cases
            if (pos >= size())
                return VectorIndex();

            if (nLen > size() - pos)
                nLen = size() - pos;

            // Return a single index
            if (nLen <= 1)
                return VectorIndex(getIndex(pos));

            // Calculate the starting and ending indices for
            // single indices. The last index is decremented,
            // because the first and the last index already
            // count as two indices.
            if (expand)
                return VectorIndex(getIndex(pos), getIndex(pos+nLen-1));

            // Return a copy of the internal storage, if it is already
            // expanded. The last index is not decremented, because
            // terminating iterator always points after the last
            // element in the list
            return VectorIndex(vector<long long int>(vStorage.begin()+pos, vStorage.begin()+pos+nLen));
        }

        /////////////////////////////////////////////////
        /// \brief This member function linearizes the
        /// contents of a vector-described index set. The
        /// vectorial information is lost after using this
        /// function and the index will be described by
        /// single indices constructed from the minimal
        /// and maximal index values.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void linearize()
        {
            if (!expand)
            {
                long long int nMin = min();
                long long int nMax = max();

                vStorage.clear();
                vStorage.assign({nMin, nMax});
                expand = true;
            }
        }

        /////////////////////////////////////////////////
        /// \brief Overload for the access operator.
        /// Redirects the control to the private
        /// getIndex() member function.
        ///
        /// \param n size_t
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        inline long long int operator[](size_t n) const
        {
            return getIndex(n);
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns the size
        /// of the indices stored in this class.
        ///
        /// It will return either the size of the stored
        /// vectorial indices or the calculated size for
        /// expanded single indices.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t size() const
        {
            if (vStorage.size() == 2 && !isValid())
                return 0;
            else if (vStorage.back() == INVALID)
                return 1;
            else if (vStorage.back() == OPEN_END)
                return -1;
            else if (vStorage.size() == 2 && expand)
                return abs(vStorage.back() - vStorage.front() + 1);
            else
                return vStorage.size();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns the
        /// number of nodes describing the index set,
        /// which is stored internally.
        ///
        /// This is used in cases, where the indices will
        /// be interpreted slightly different than in the
        /// usual cases.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t numberOfNodes() const
        {
            if (!isValid())
                return 0;
            else if (vStorage.size() == 2 && ((vStorage.back() == INVALID) xor (vStorage.front() == INVALID)))
                return 1;
            else
                return vStorage.size();
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the single indices are in the correct
        /// order.
        ///
        /// This function won't return correct results for
        /// already expanded vectors.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isOrdered() const
        {
            if (isValid())
                return vStorage.front() <= vStorage.back() || vStorage.back() == INVALID || vStorage.back() == OPEN_END;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the indices are calculated or actual
        /// vectorial indices.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isExpanded() const
        {
            return expand;
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// set the index at a special position. This
        /// will expand the internal storage and switch
        /// from single indices to vectorial ones
        /// automatically.
        ///
        /// \param nthIndex size_t
        /// \param nVal long long int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setIndex(size_t nthIndex, long long int nVal)
        {
            // If the size is large enough, simply store
            // the passed index. Otherwise expand the
            // index using invalid values and store it
            // afterwards
            if (nthIndex < vStorage.size())
            {
                vStorage[nthIndex] = nVal;
            }
            else
            {
                // increase the size by using invalid values
                while (vStorage.size() <= nthIndex)
                    vStorage.push_back(INVALID);

                vStorage[nthIndex] = nVal;

                // If the last value is not an open end
                // value and the size of the internal storage
                // is larger than two, deactivate the expanding
                if (vStorage.size() > 2 && vStorage.back() != OPEN_END)
                    expand = false;
            }
        }

        /////////////////////////////////////////////////
        /// \brief This function will append the passed
        /// index value at the end of the index vector.
        /// The internal storage is expanded first, if
        /// necessary.
        ///
        /// \param nVal long long int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void push_back(long long int nVal)
        {
            if (expand)
            {
                vStorage = getVector();
                expand = false;
            }

            vStorage.push_back(nVal);
        }

        /////////////////////////////////////////////////
        /// \brief This function will append the passed
        /// vector to the end of the index vector. The
        /// internal storage is expanded first, if
        /// necessary.
        ///
        /// \param vVector const vector<long long int>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void append(const vector<long long int>& vVector)
        {
            if (expand)
            {
                vStorage = getVector();
                expand = false;
            }

            vStorage.insert(vStorage.end(), vVector.begin(), vVector.end());
        }

        /////////////////////////////////////////////////
        /// \brief This function will append the passed
        /// VectorIndex to the end of the index vector.
        /// The internal storage is expanded first, if
        /// necessary. This function is an overload for
        /// convenience.
        ///
        /// \param vIndex const VectorIndex&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void append(const VectorIndex& vIndex)
        {
            if (vIndex.expand)
                append(vIndex.getVector());
            else
                append(vIndex.vStorage);
        }

        /////////////////////////////////////////////////
        /// \brief This function will prepend the passed
        /// vector before the beginning of the index
        /// vector. The internal storage is expanded
        /// first, if necessary.
        ///
        /// \param vVector const vector<long long int>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void prepend(const vector<long long int>& vVector)
        {
            if (expand)
            {
                vStorage = getVector();
                expand = false;
            }

            vStorage.insert(vStorage.begin(), vVector.begin(), vVector.end());
        }

        /////////////////////////////////////////////////
        /// \brief This function will append the passed
        /// VectorIndex before the beginning of the index
        /// vector. The internal storage is expanded
        /// first, if necessary. This function is an
        /// overload for convenience.
        ///
        /// \param vIndex const VectorIndex&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void prepend(const VectorIndex& vIndex)
        {
            if (vIndex.expand)
                prepend(vIndex.getVector());
            else
                prepend(vIndex.vStorage);
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a STL
        /// vector, which will resemble the indices
        /// stored internally. This includes that the
        /// single indices are expanded in the returned
        /// vector.
        ///
        /// \return vector<long long int>
        ///
        /////////////////////////////////////////////////
        vector<long long int> getVector() const
        {
            // Expand the single indices stored internally
            // if needed
            if (expand)
            {
                vector<long long int> vReturn;

                for (size_t i = 0; i < size(); i++)
                {
                    vReturn.push_back(getIndex(i));
                }

                return vReturn;
            }

            return vStorage;
        }

        /////////////////////////////////////////////////
        /// \brief This function calculates the maximal
        /// index value obtained from the values stored
        /// internally.
        ///
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        long long int max() const
        {
            if (expand)
                return ::max(vStorage.front(), vStorage.back());

            long long int nMax = vStorage.front();

            for (size_t i = 1; i < vStorage.size(); i++)
            {
                if (vStorage[i] > nMax)
                    nMax = vStorage[i];
            }

            return nMax;
        }

        /////////////////////////////////////////////////
        /// \brief This member function calculates the
        /// minimal index value obtained from the values
        /// stored internally.
        ///
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        long long int min() const
        {
            if (expand)
                return ::min(vStorage.front(), vStorage.back());

            long long int nMin = vStorage.front();

            for (size_t i = 1; i < vStorage.size(); i++)
            {
                if (vStorage[i] > nMin)
                    nMin = vStorage[i];
            }

            return nMin;
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the internal index set is valid.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool isValid() const
        {
            return vStorage.front() != INVALID || vStorage.back() != INVALID;
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the internal index set has an open
        /// end.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool isOpenEnd() const
        {
            return vStorage.back() == OPEN_END;
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the internal index set referres to
        /// the table headlines.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool isString() const
        {
            return vStorage.front() == STRING || vStorage.back() == STRING;
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a
        /// reference to the first index value stored
        /// internally.
        ///
        /// \return long long int&
        ///
        /////////////////////////////////////////////////
        long long int& front()
        {
            return vStorage.front();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a
        /// reference to the final index value stored
        /// internally.
        ///
        /// \return long long int&
        ///
        /////////////////////////////////////////////////
        long long int& back()
        {
            return vStorage.back();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a const
        /// reference to the first index value stored
        /// internally.
        ///
        /// \return const long long int&
        ///
        /////////////////////////////////////////////////
        const long long int& front() const
        {
            return vStorage.front();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a const
        /// reference to the final index value stored
        /// internally.
        ///
        /// \return const long long int&
        ///
        /////////////////////////////////////////////////
        const long long int& back() const
        {
            return vStorage.back();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns the last
        /// index value, which can be reached by the
        /// values stored internally (this is most
        /// probably different from the final value).
        ///
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        long long int last() const
        {
            if (expand && vStorage.back() == INVALID)
                return vStorage.front();

            return vStorage.back();
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// force the indices stored internally to be in
        /// a defined interval. If the values are already
        /// in a smaller interval, nothing happens.
        ///
        /// \param nMin long long int
        /// \param nMax long long int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setRange(long long int nMin, long long int nMax)
        {
            // Change the order of the minimal and
            // maximal value, if needed
            if (nMin > nMax)
            {
                long long int nTemp = nMin;
                nMin = nMax;
                nMax = nTemp;
            }

            // Compare all values to the defined
            // interval
            for (size_t i = 0; i < vStorage.size(); i++)
            {
                // Special cases: if the current value
                // is open end, use the maximal value
                // passed to this function
                if (vStorage[i] == OPEN_END)
                {
                    vStorage[i] = nMax;
                    continue;
                }
                else if (vStorage[i] == INVALID)
                    continue;

                if (vStorage[i] < nMin)
                    vStorage[i] = nMin;

                if (vStorage[i] > nMax)
                    vStorage[i] = nMax;
            }
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// replace the open end state with a defined
        /// index value although the VectorIndex instance
        /// was passed as const ref.
        ///
        /// \param nLast long longint
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setOpenEndIndex(long long int nLast) const
        {
            if (vStorage.back() == OPEN_END)
                vStorage.back() = nLast;
        }

        /////////////////////////////////////////////////
        /// \brief This member function converts the
        /// vector indexes contents into a human-readable
        /// string representation.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        std::string to_string() const
        {
            if (expand)
            {
                std::string sVect;

                if (vStorage.front() == INVALID)
                    sVect += "INVALID";
                else if (vStorage.front() == STRING)
                    sVect += "#";
                else
                    sVect += toString(vStorage.front()+1);

                // Do not present the second index if
                // it corresponds to an invalid value
                if (vStorage.back() == OPEN_END)
                    sVect += ":inf";
                else if (vStorage.back() == STRING)
                    sVect += ":#";
                else if (vStorage.back() != INVALID)
                    sVect += ":"+toString(vStorage.back()+1);

                return sVect;
            }
            else
            {
                std::string sVect = "{";

                for (size_t i = 0; i < size(); i++)
                {
                    // Jump over the middle section,
                    // if the vector length is larger
                    // than 5
                    if (size() >= 5u && i == 2)
                    {
                        sVect += "...,";
                        i = size() - 2;
                    }

                    if (getIndex(i) == INVALID)
                        sVect += "INVALID";
                    else if (getIndex(i) == OPEN_END)
                        sVect += "inf";
                    else if (getIndex(i) == STRING)
                        sVect += "#";
                    else
                        sVect += toString(getIndex(i)+1);

                    if (i + 1 < size())
                        sVect += ",";
                }

                return sVect + "}";
            }
        }
};


/////////////////////////////////////////////////
/// \brief This class extends the std::vector for
/// endlessness.
///
/// This class template automatically creates
/// empty elements, if the index operator access
/// elements beyond its size.
/////////////////////////////////////////////////
template<class T>
class EndlessVector : public vector<T>
{
    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor
        /////////////////////////////////////////////////
        EndlessVector() : vector<T>() {}

        /////////////////////////////////////////////////
        /// \brief Copy constructor from same.
        ///
        /// \param vec const EndlessVector&
        ///
        /////////////////////////////////////////////////
        EndlessVector(const EndlessVector& vec) : vector<T>(vec) {}

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload from
        /// same.
        ///
        /// \param vec const EndlessVector&
        /// \return EndlessVector&
        ///
        /////////////////////////////////////////////////
        EndlessVector& operator=(const EndlessVector& vec)
        {
            vector<T>::operator=(vec);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Access operator overload. Will return
        /// default constructed instances of the template
        /// type T, if the position n points to a
        /// position beyond the internal array.
        ///
        /// \param n size_t
        /// \return T
        ///
        /////////////////////////////////////////////////
        T operator[](size_t n)
        {
            if (n < vector<T>::size())
                return vector<T>::operator[](n);

            return T();
        }
};


/////////////////////////////////////////////////
/// \brief This class is a base class for all
/// string view classes.
///
/// It gathers the common const
/// operations like finding and equality
/// operators. It can be instantated directly,
/// but it will do nothing due to a missing
/// pointer to the viewed std::string.
///
/// \note String view classes are neither thread
/// safe nor do they update, when the data source
/// has been altered in other locations. They can
/// only be used in single-thread contexts and
/// should be considered as immutable. The
/// MutableStringView class can handle some
/// modifications but will probably invalidate
/// all other string views using the same data
/// source while modifying the data.
/////////////////////////////////////////////////
class StringViewBase
{
    protected:
        size_t m_start;
        size_t m_len;

        /////////////////////////////////////////////////
        /// \brief This private member function
        /// evaluates, whether the passed length is part
        /// of the viewed section and adapts the length
        /// correspondingly.
        ///
        /// \param pos size_t starting position
        /// \param len size_t
        /// \return size_t the new length
        ///
        /////////////////////////////////////////////////
        inline size_t validizeLength(size_t pos, size_t len) const
        {
            if (len == std::string::npos || pos+len > m_len)
                len = m_len - pos;

            return len;
        }

        /////////////////////////////////////////////////
        /// \brief Reset function.
        ///
        /// \return virtual  void
        ///
        /////////////////////////////////////////////////
        virtual inline void clear()
        {
            m_start = 0;
            m_len = 0;
        }

        /////////////////////////////////////////////////
        /// \brief This member function checks, whether
        /// the passed (absolute) position is part of the
        /// viewed string section. Is mostly used in
        /// string find operations.
        ///
        /// \param pos size_t
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool validAbsolutePosition(size_t pos) const
        {
            return pos >= m_start && pos < m_start + m_len;
        }

    public:

        /////////////////////////////////////////////////
        /// \brief StringViewBase default constructor.
        /////////////////////////////////////////////////
        StringViewBase() : m_start(0), m_len(0) {}

        /////////////////////////////////////////////////
        /// \brief This member function returns a const
        /// pointer to the viewed string. Is only used
        /// internally.
        ///
        /// \return const std::string*
        ///
        /////////////////////////////////////////////////
        virtual inline const std::string* getData() const
        {
            return nullptr;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the equality operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator==(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return getData()->compare(m_start, m_len, *view.getData(), view.m_start, view.m_len) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the equality operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator==(const std::string& sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) == 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the inequality operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator!=(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return getData()->compare(m_start, m_len, *view.getData(), view.m_start, view.m_len) != 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the inequality operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator!=(const std::string& sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) != 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the less operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator<(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return getData()->compare(m_start, m_len, *view.getData(), view.m_start, view.m_len) < 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the less operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator<(const std::string& sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) < 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the greater operator using another
        /// StringViewBase instance.
        ///
        /// \param view const StringViewBase&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator>(const StringViewBase& view) const
        {
            if (getData() && view.getData())
                return getData()->compare(m_start, m_len, *view.getData(), view.m_start, view.m_len) > 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function is an overload
        /// for the greater operator using a const
        /// std::string instance.
        ///
        /// \param sString const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool operator>(const std::string& sString) const
        {
            if (getData())
                return getData()->compare(m_start, m_len, sString) > 0;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function provides a const
        /// char reference to the first character in the
        /// viewed section.
        ///
        /// \return const char&
        ///
        /////////////////////////////////////////////////
        inline const char& front() const
        {
            if (getData())
                return getData()->at(m_start);

            throw std::out_of_range("StringView::front");
        }

        /////////////////////////////////////////////////
        /// \brief This member function provides a const
        /// char reference to the last character in the
        /// viewed section.
        ///
        /// \return const char&
        ///
        /////////////////////////////////////////////////
        inline const char& back() const
        {
            if (getData())
                return getData()->at(m_start+m_len-1);

            throw std::out_of_range("StringView::back");
        }

        /////////////////////////////////////////////////
        /// \brief This member function provides an
        /// iterator to the beginning of the viewed
        /// section of the internal string.
        ///
        /// \return std::string::const_iterator
        ///
        /////////////////////////////////////////////////
        inline std::string::const_iterator begin() const
        {
            if (getData())
                return getData()->begin() + m_start;

            return std::string::const_iterator();
        }

        /////////////////////////////////////////////////
        /// \brief This member function provides an
        /// iterator to the end of the viewed section of
        /// the internal string.
        ///
        /// \return std::string::const_iterator
        ///
        /////////////////////////////////////////////////
        inline std::string::const_iterator end() const
        {
            if (getData())
                return getData()->begin() + m_start + m_len;

            return std::string::const_iterator();
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// remove characters from the front of the
        /// viewed section.
        ///
        /// \param len size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void trim_front(size_t len)
        {
            if (len < m_len)
            {
                m_start += len;
                m_len -= len;
            }
            else
                clear();
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// remove characters from the back of the
        /// viewed section.
        ///
        /// \param len size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void trim_back(size_t len)
        {
            if (len < m_len)
                m_len -= len;
            else
                clear();
        }

        /////////////////////////////////////////////////
        /// \brief This member function shrinks the
        /// viewed section to remove all leading or
        /// trailing whitespace characters. This is the
        /// corresponding member function to
        /// StripSpaces(std::string&).
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void strip()
        {
            const std::string* data = getData();

            if (!data)
                return;

            // Strip leading
            while (m_len && isblank(data->operator[](m_start)))
            {
                m_start++;
                m_len--;
            }

            if (!m_len)
            {
                clear();
                return;
            }

            // Strip trailing
            while (m_len && isblank(data->operator[](m_start+m_len-1)))
            {
                m_len--;
            }
        }

        /////////////////////////////////////////////////
        /// \brief This member function simply returns
        /// the length of the viewed section.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        inline size_t length() const
        {
            return m_len;
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a copy of
        /// the viewed section of the string
        /// (via std::string::substr). Note that this is
        /// an inefficient operation.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string to_string() const
        {
            if (getData())
                return getData()->substr(m_start, m_len);

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find(const std::string& findstr, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find()
        ///
        /// \param c char
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find(char c, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find(c, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::rfind()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t rfind(const std::string& findstr, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->rfind(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::rfind()
        ///
        /// \param c char
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t rfind(char c, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->rfind(c, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_first_of()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_first_of(const std::string& findstr, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find_first_of(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_first_not_of()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_first_not_of(const std::string& findstr, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find_first_not_of(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_first_not_of()
        ///
        /// \param c char
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_first_not_of(char c, size_t pos = 0) const
        {
            if (getData())
            {
                size_t fnd = getData()->find_first_not_of(c, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_last_of()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_last_of(const std::string& findstr, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->find_last_of(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

        /////////////////////////////////////////////////
        /// \brief Wrapper member function for
        /// std::string::find_last_not_of()
        ///
        /// \param findstr const std::string&
        /// \param pos size_t
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t find_last_not_of(const std::string& findstr, size_t pos = std::string::npos) const
        {
            if (getData())
            {
                pos = validizeLength(m_start, pos);
                size_t fnd = getData()->find_last_not_of(findstr, m_start + pos);

                if (validAbsolutePosition(fnd))
                    return fnd-m_start;
            }

            return std::string::npos;
        }

};


// Forward declaration for friendship
class StringView;


/////////////////////////////////////////////////
/// \brief This class is a mutable version of a
/// string view. It can be used to replace single
/// characters or entire parts of the string.
/// It's possible to convert a MutableStringView
/// into a (const) StringView but not the other
/// way around.
/////////////////////////////////////////////////
class MutableStringView : public StringViewBase
{
    private:
        friend class StringView;
        std::string* m_data;

        /////////////////////////////////////////////////
        /// \brief Private constructor used by the
        /// subview member function.
        ///
        /// \param data std::string*
        /// \param start size_t
        /// \param len size_t
        ///
        /////////////////////////////////////////////////
        MutableStringView(std::string* data, size_t start, size_t len) : StringViewBase(), m_data(data)
        {
            m_start = start;
            m_len = len;
        }

    protected:
        /////////////////////////////////////////////////
        /// \brief Override to return a pointer to the
        /// internal string.
        ///
        /// \return const std::string*
        ///
        /////////////////////////////////////////////////
        virtual inline const std::string* getData() const override
        {
            return m_data;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from
        /// another MutableStringView instance.
        ///
        /// \param view MutableStringView&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(MutableStringView& view)
        {
            m_data = view.m_data;
            m_start = view.m_start;
            m_len = view.m_len;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from a
        /// std::string pointer.
        ///
        /// \param data std::string*
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(std::string* data)
        {
            if (!data)
                return;

            m_data = data;
            m_start = 0;
            m_len = m_data->length();
        }

        /////////////////////////////////////////////////
        /// \brief Override to clear the internal pointer
        /// as well.
        ///
        /// \return virtual  void
        ///
        /////////////////////////////////////////////////
        virtual inline void clear() override
        {
            m_data = nullptr;
            m_start = 0;
            m_len = 0;
        }

    public:
        /////////////////////////////////////////////////
        /// \brief MutableStringView default constructor.
        /////////////////////////////////////////////////
        MutableStringView() : StringViewBase(), m_data(nullptr) {}

        /////////////////////////////////////////////////
        /// \brief MutableStringView constructor from a
        /// std::string pointer.
        ///
        /// \param data std::string*
        ///
        /////////////////////////////////////////////////
        MutableStringView(std::string* data) : MutableStringView()
        {
            assign(data);
        }

        /////////////////////////////////////////////////
        /// \brief MutableStringView constructor from a
        /// (non-const) std::string reference.
        ///
        /// \param data std::string&
        ///
        /////////////////////////////////////////////////
        MutableStringView(std::string& data) : MutableStringView()
        {
            assign(&data);
        }

        /////////////////////////////////////////////////
        /// \brief MutableStringView copy constrcutor.
        ///
        /// \param view MutableStringView&
        ///
        /////////////////////////////////////////////////
        MutableStringView(MutableStringView& view) : MutableStringView()
        {
            assign(view);
        }

        /////////////////////////////////////////////////
        /// \brief MutableStringView move constructor.
        ///
        /// \param view MutableStringView&&
        ///
        /////////////////////////////////////////////////
        MutableStringView(MutableStringView&& view)
        {
            m_data = std::move(view.m_data);
            m_start = std::move(view.m_start);
            m_len = std::move(view.m_len);
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for another
        /// MutableStringView instance.
        ///
        /// \param view MutableStringView&
        /// \return MutableStringView&
        ///
        /////////////////////////////////////////////////
        MutableStringView& operator=(MutableStringView& view)
        {
            assign(view);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a std::string
        /// pointer.
        ///
        /// \param data std::string*
        /// \return MutableStringView&
        ///
        /////////////////////////////////////////////////
        MutableStringView& operator=(std::string* data)
        {
            assign(data);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a std::string
        /// (non-const) reference.
        ///
        /// \param data std::string&
        /// \return MutableStringView&
        ///
        /////////////////////////////////////////////////
        MutableStringView& operator=(std::string& data)
        {
            assign(&data);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Implementation of the random access
        /// operator. Returns a (non-const) char
        /// reference.
        ///
        /// \param pos size_t
        /// \return char&
        /// \remark No boundary checks are performed.
        ///
        /////////////////////////////////////////////////
        inline char& operator[](size_t pos)
        {
            return m_data->operator[](m_start+pos);
        }

        /////////////////////////////////////////////////
        /// \brief This member function creates a new
        /// MutableStringView class instance using the
        /// selected position and length as a new viewed
        /// part. The position has to be part of the
        /// viewed section of this instance. This
        /// function can be used to replace
        /// std::string::substr.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \return MutableStringView
        ///
        /////////////////////////////////////////////////
        MutableStringView subview(size_t pos = 0, size_t len = std::string::npos) const
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);
                return MutableStringView(m_data, m_start+pos, len);
            }

            return MutableStringView();
        }

        /////////////////////////////////////////////////
        /// \brief This member function replaces a range
        /// in the internal viewed string with the passed
        /// string.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \param s const std::string&
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& replace(size_t pos, size_t len, const std::string& s)
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);
                m_data->replace(m_start+pos, len, s);
                m_len += s.length() - len;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function replaces a range
        /// in the internal viewed string with the passed
        /// string. This function allows to select a
        /// smaller part of the replacing string.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \param s const std::string&
        /// \param subpos size_t
        /// \param sublen size_t
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& replace(size_t pos, size_t len, const std::string& s, size_t subpos, size_t sublen)
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);

                if (subpos + sublen > s.length())
                    sublen = s.length() - subpos;

                m_data->replace(m_start+pos, len, s, subpos, sublen);
                m_len += sublen - len;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function replaces a range
        /// in the internal viewed string with the passed
        /// StringViewBase.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \param view const StringViewBase&
        /// \return MutableStringView&
        /// \remark Positions and lengths of other
        /// StringViews to the same strings are
        /// invalidated, if the lengths of the replaced
        /// and the replacing string differ.
        ///
        /////////////////////////////////////////////////
        MutableStringView& replace(size_t pos, size_t len, const StringViewBase& view)
        {
            return replace(pos, len, view.to_string());
        }

};


/////////////////////////////////////////////////
/// \brief This class is the immutable (const)
/// version of a string view. It can be
/// constructed from a MutableStringView, but
/// cannot be used to construct a mutable
/// version.
/////////////////////////////////////////////////
class StringView : public StringViewBase
{
    private:
        const std::string* m_data;

        /////////////////////////////////////////////////
        /// \brief Private constructor used by the
        /// subview member function.
        ///
        /// \param data std::string* const
        /// \param start size_t
        /// \param len size_t
        ///
        /////////////////////////////////////////////////
        StringView(const std::string* data, size_t start, size_t len) : StringViewBase(), m_data(data)
        {
            m_start = start;
            m_len = len;
        }

    protected:
        /////////////////////////////////////////////////
        /// \brief Override to return a pointer to the
        /// internal string.
        ///
        /// \return const std::string*
        ///
        /////////////////////////////////////////////////
        virtual inline const std::string* getData() const override
        {
            return m_data;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from
        /// another StringView instance.
        ///
        /// \param view const StringView&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(const StringView& view)
        {
            m_data = view.m_data;
            m_start = view.m_start;
            m_len = view.m_len;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from a
        /// MutableStringView instance.
        ///
        /// \param view const MutableStringView&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(const MutableStringView& view)
        {
            m_data = view.m_data;
            m_start = view.m_start;
            m_len = view.m_len;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment member function from a
        /// const std::string pointer.
        ///
        /// \param data const std::string*
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(const std::string* data)
        {
            if (!data)
                return;

            m_data = data;
            m_start = 0;
            m_len = m_data->length();
        }

        /////////////////////////////////////////////////
        /// \brief Override to clear the internal pointer
        /// as well.
        ///
        /// \return virtual  void
        ///
        /////////////////////////////////////////////////
        virtual inline void clear() override
        {
            m_data = nullptr;
            m_start = 0;
            m_len = 0;
        }

    public:
        /////////////////////////////////////////////////
        /// \brief StringView default constructor.
        /////////////////////////////////////////////////
        StringView() : StringViewBase(), m_data(nullptr) {}

        /////////////////////////////////////////////////
        /// \brief StringView constructor from a const
        /// std::string pointer.
        ///
        /// \param data const std::string*
        ///
        /////////////////////////////////////////////////
        StringView(const std::string* data) : StringView()
        {
            assign(data);
        }

        /////////////////////////////////////////////////
        /// \brief StringView constructor from a const
        /// std::string reference.
        ///
        /// \param data const std::string&
        ///
        /////////////////////////////////////////////////
        StringView(const std::string& data) : StringView()
        {
            assign(&data);
        }

        /////////////////////////////////////////////////
        /// \brief StringView copy constructor.
        ///
        /// \param view const StringView&
        ///
        /////////////////////////////////////////////////
        StringView(const StringView& view) : StringView()
        {
            assign(view);
        }

        /////////////////////////////////////////////////
        /// \brief StringView constructor from a
        /// MutableStringView class instance.
        ///
        /// \param view const MutableStringView&
        ///
        /////////////////////////////////////////////////
        StringView(const MutableStringView& view) : StringView()
        {
            assign(view);
        }

        /////////////////////////////////////////////////
        /// \brief StringView move constructor.
        ///
        /// \param view StringView&&
        ///
        /////////////////////////////////////////////////
        StringView(StringView&& view)
        {
            m_data = std::move(view.m_data);
            m_start = std::move(view.m_start);
            m_len = std::move(view.m_len);
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for another
        /// StringView instance.
        ///
        /// \param view const StringView&
        /// \return StringView&
        ///
        /////////////////////////////////////////////////
        StringView& operator=(const StringView& view)
        {
            assign(view);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a
        /// MutableStringView class instance.
        ///
        /// \param view const MutableStringView&
        /// \return StringView&
        ///
        /////////////////////////////////////////////////
        StringView& operator=(const MutableStringView& view)
        {
            assign(view);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a const
        /// std::string pointer.
        ///
        /// \param data const std::string*
        /// \return StringView&
        ///
        /////////////////////////////////////////////////
        StringView& operator=(const std::string* data)
        {
            assign(data);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator for a const
        /// std::string reference.
        ///
        /// \param data const std::string&
        /// \return StringView&
        ///
        /////////////////////////////////////////////////
        StringView& operator=(const std::string& data)
        {
            assign(&data);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Random access operator, returning a
        /// const char reference.
        ///
        /// \param pos size_t
        /// \return const char&
        /// \remark No boundary checks are performed.
        ///
        /////////////////////////////////////////////////
        inline const char& operator[](size_t pos) const
        {
            return m_data->operator[](m_start+pos);
        }

        /////////////////////////////////////////////////
        /// \brief This member function creates a new
        /// StringView class instance using the selected
        /// position and length as a new viewed part. The
        /// position has to be part of the viewed section
        /// of this instance. This function can be used
        /// to replace std::string::substr.
        ///
        /// \param pos size_t
        /// \param len size_t
        /// \return StringView
        ///
        /////////////////////////////////////////////////
        StringView subview(size_t pos = 0, size_t len = std::string::npos) const
        {
            if (m_data && pos < m_len)
            {
                len = validizeLength(pos, len);
                return StringView(m_data, m_start+pos, len);
            }

            return StringView();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a
        /// MutableStringView instance with the data of
        /// this instance.
        ///
        /// \return MutableStringView
        /// \warning Only use this method, if you know,
        /// what you're doing.
        ///
        /////////////////////////////////////////////////
        MutableStringView make_mutable() const
        {
            if (m_data)
                return MutableStringView(const_cast<std::string*>(m_data), m_start, m_len);

            return MutableStringView();
        }

};


/////////////////////////////////////////////////
/// \brief This structure is central for managing
/// the indices of a table or cluster read or
/// write data access. The contained indices are
/// of the VectorIndex type. The structure also
/// contains a precompiled data access equation,
/// which is used to speed up the data access in
/// loops.
/////////////////////////////////////////////////
struct Indices
{
    VectorIndex row;
    VectorIndex col;
    string sCompiledAccessEquation;

    Indices() { }
    Indices(const Indices& _idx) : row(_idx.row), col(_idx.col), sCompiledAccessEquation(_idx.sCompiledAccessEquation)
    {
    }
    Indices(Indices&& _idx)
    {
        std::swap(_idx.row, row);
        std::swap(_idx.col, col);
        std::swap(_idx.sCompiledAccessEquation, sCompiledAccessEquation);
    }
    Indices& operator=(const Indices& _idx)
    {
        row = _idx.row;
        col = _idx.col;
        sCompiledAccessEquation = _idx.sCompiledAccessEquation;
        return *this;
    }
};


/////////////////////////////////////////////////
/// \brief Structure for the findCommand
/// function.
/////////////////////////////////////////////////
struct Match
{
    string sString;
    unsigned int nPos;
};


/////////////////////////////////////////////////
/// \brief This represents a point in 2D space.
/////////////////////////////////////////////////
struct Point
{
    double x;
    double y;

    Point(double _x, double _y) : x(_x), y(_y) {}

    void rotate(double dAlpha, const Point& origin = Point(0, 0))
    {
        double x1 = (x - origin.x) * cos(dAlpha) - (y - origin.y) * sin(dAlpha) + origin.x;
        double y1 = (x - origin.x) * sin(dAlpha) + (y - origin.y) * cos(dAlpha) + origin.y;

        x = x1;
        y = y1;
    }

    Point operator+(const Point& a) const
    {
        return Point(x + a.x, y + a.y);
    }

    Point operator-(const Point& a) const
    {
        return Point(x - a.x, y - a.y);
    }

    Point operator*(double a) const
    {
        return Point(x*a, y*a);
    }

    Point operator/(double a) const
    {
        return Point(x/a, y/a);
    }
};


/////////////////////////////////////////////////
/// \brief Structure for the horizontal and
/// vertical lines in plots.
/////////////////////////////////////////////////
struct Line
{
    string sDesc;
    string sStyle;
    double dPos;

    Line() : sDesc(""), sStyle("k;2"), dPos(0.0) {}
};


/////////////////////////////////////////////////
/// \brief Structure for the axes in plots.
/////////////////////////////////////////////////
struct Axis
{
    string sLabel;
    string sStyle;
    double dMin;
    double dMax;
};


/////////////////////////////////////////////////
/// \brief Structure for describing time axes in
/// plots.
/////////////////////////////////////////////////
struct TimeAxis
{
    string sTimeFormat;
    bool use;

    TimeAxis() : sTimeFormat(""), use(false) {}

    void activate(const string& sFormat = "")
    {
        use = true;
        sTimeFormat = sFormat;

        if (!sTimeFormat.length())
            return;

        if (sTimeFormat.find("YYYY") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("YYYY"), 4, "%Y");

        if (sTimeFormat.find("YY") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("YY"), 2, "%y");

        if (sTimeFormat.find("MM") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("MM"), 2, "%m");

        if (sTimeFormat.find("DD") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("DD"), 2, "%d");

        if (sTimeFormat.find("HH") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("HH"), 2, "%H");

        if (sTimeFormat.find("hh") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("hh"), 2, "%H");

        if (sTimeFormat.find("mm") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("mm"), 2, "%M");

        if (sTimeFormat.find("ss") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("ss"), 2, "%S");
    }

    void deactivate()
    {
        use = false;
        sTimeFormat.clear();
    }
};


/////////////////////////////////////////////////
/// \brief Structure as wrapper for the return
/// value of procedures (which may be numerical
/// or string values or a mixture of both).
/////////////////////////////////////////////////
struct Returnvalue
{
    vector<double> vNumVal;
    vector<string> vStringVal;

    // clear method
    void clear()
    {
        vNumVal.clear();
        vStringVal.clear();
    }

    // boolean checkers
    bool isString() const
    {
        return vStringVal.size();
    }
    bool isNumeric() const
    {
        return vNumVal.size() && !vStringVal.size();
    }
};


/////////////////////////////////////////////////
/// \brief Structure for the four standard
/// variables.
/////////////////////////////////////////////////
struct DefaultVariables
{
    string sName[4] = {"x", "y", "z", "t"};
    double vValue[4][4];
};


/////////////////////////////////////////////////
/// \brief Structure for the sorting
/// functionality: used for the recursive
/// definition of the index columns for sorting.
/////////////////////////////////////////////////
struct ColumnKeys
{
    int nKey[2];
    // Contains a recursive pointer
    ColumnKeys* subkeys;

    // Default constructor
    ColumnKeys() : nKey{-1,-1}, subkeys(nullptr) {}
    // Destructor recursively deletes the stored pointers
    ~ColumnKeys()
        {
            if (subkeys)
                delete subkeys;
        }
};


/////////////////////////////////////////////////
/// \brief This structure contains the
/// information of a two-dimensional boundary.
/////////////////////////////////////////////////
struct Boundary
{
    long long int n;
    long long int m;
    size_t rows;
    size_t cols;

    Boundary(long long int i, long long int j, size_t _row, size_t _col) : n(i), m(j), rows(_row), cols(_col) {}

    long long int rf()
    {
        return n;
    }

    long long int re()
    {
        return n+rows;
    }

    long long int cf()
    {
        return m;
    }

    long long int ce()
    {
        return m+cols;
    }
};

#endif
